require("./style.scss");

import * as d3 from "d3";
import * as Plot from "@observablehq/plot";

import { encoded_query_response, entry, query_response, query_result } from "../server/schema.js";
import { debounce, http_get } from "./utils";
import { occlusionY } from "./occlusion";

class App {
    static readonly first_bucket: [number, number] = [2017, 6]; // july 2017
    query_input: HTMLInputElement;
    case_insensitive_button: HTMLElement;
    combine_button: HTMLElement;
    error: HTMLElement;
    chart: HTMLElement;
    timing: HTMLElement;

    query: string;
    case_insensitive = false;
    combine = false;

    constructor() {
        this.query_input = document.getElementById("query")! as HTMLInputElement;
        this.query = this.query_input.value;
        this.query_input.addEventListener("keyup", debounce(this.query_keystroke.bind(this), 500), false);
        this.case_insensitive_button = document.getElementById("case-insensitive")!;
        this.case_insensitive_button.addEventListener("click", this.case_insensitive_button_press.bind(this), false);
        this.combine_button = document.getElementById("combine-series")!;
        this.combine_button.addEventListener("click", this.combine_button_press.bind(this), false);
        this.error = document.getElementById("error")!;
        this.chart = document.getElementById("chart")!;
        this.timing = document.getElementById("timing")!;
        this.do_query();
    }

    case_insensitive_button_press() {
        this.case_insensitive = !this.case_insensitive;
        this.case_insensitive_button.setAttribute("class", this.case_insensitive ? "on" : "");
        this.do_query();
    }

    combine_button_press() {
        this.combine = !this.combine;
        this.combine_button.setAttribute("class", this.combine ? "on" : "");
        this.do_query();
    }

    query_keystroke() {
        this.query = this.query_input.value;
        this.do_query();
    }

    static months_after_first_bucket(months: number) {
        return Date.UTC(this.first_bucket[0], this.first_bucket[1] + months);
    }

    prepare_data(response: encoded_query_response) {
        // we get an array of results for each part of the query
        // we want to consolidate down to a single dict, keeping the order (we use that for the color domain)
        this.timing.innerHTML = `Query time: ${response.time} ms`;
        const consolidated_result: query_result = new Map(response.series.flat(1));
        const last_bucket = Math.max(
            ...[...consolidated_result.values()].map(series => Math.max(...series.map(entry => entry.year_month))),
        );
        // fill in gaps
        for (const series of consolidated_result.values()) {
            series.sort((a, b) => a.year_month - b.year_month);
            // there should be an entry for each moth from first bucket to last bucket, fill in gaps with 0's
            for (let month = 0; App.months_after_first_bucket(month) <= last_bucket; month++) {
                if (month >= series.length || App.months_after_first_bucket(month) < series[month].year_month) {
                    series.splice(month, 0, { year_month: App.months_after_first_bucket(month), frequency: 0 });
                }
            }
        }
        // pivot
        const data = [...consolidated_result.entries()]
            .map(([ngram, entries]) => {
                return entries.map(({ year_month, frequency }) => {
                    return { date: new Date(year_month), frequency, ngram };
                });
            })
            .flat();
        return [[...consolidated_result.keys()], data] as [
            string[],
            {
                date: Date;
                frequency: number;
                ngram: string;
            }[],
        ];
    }

    render_chart(raw_data: encoded_query_response) {
        const [domain, data] = this.prepare_data(raw_data);
        const plot = Plot.plot({
            grid: true,
            width: 1500,
            height: 800,
            style: { fontSize: "16px", overflow: "visible" },
            marginLeft: 50,
            marginBottom: 50,
            marginRight: 100,
            color: { legend: data.length > 0, className: "legend-text", domain } as Plot.ScaleOptions,
            marks: [
                Plot.lineY(data, {
                    x: "date",
                    y: "frequency",
                    stroke: "ngram",
                    z: "ngram",
                }),
                Plot.axisY({ label: "Frequency", marginBottom: 10, tickFormat: value => value.toExponential(0) }),
                Plot.text(
                    data,
                    occlusionY(
                        { minDistance: 20 },
                        Plot.selectLast({
                            x: "date",
                            y: "frequency",
                            z: "ngram",
                            text: "ngram",
                            fill: "ngram",
                            textAnchor: "start",
                            dx: 3,
                        }),
                    ),
                ),
                Plot.ruleX(data, Plot.pointerX(Plot.binX({}, { x: "date", thresholds: 10000, insetTop: 20 }))),
                // https://github.com/observablehq/plot/issues/2003
                Plot.tip(
                    data,
                    Plot.pointerX(
                        Plot.binX(
                            {
                                y: () => 0,
                                title: (v: any) => v,
                            },
                            {
                                x: "date",
                                y: "frequency",
                                thresholds: 1000,
                                render: (
                                    index: number[],
                                    scales: Plot.ScaleFunctions,
                                    values: Plot.ChannelValues,
                                    dimensions: Plot.Dimensions,
                                    context: Plot.Context,
                                ) => {
                                    const g = d3.select(context.ownerSVGElement).append("g");
                                    const [i] = index;
                                    if (i !== undefined) {
                                        const data = values.title![i];
                                        g.attr(
                                            "transform",
                                            `translate(${Math.min(
                                                values.x1![i],
                                                dimensions.width - dimensions.marginRight - 200,
                                            )}, 20)`,
                                        ).append(() =>
                                            Plot.plot({
                                                marginTop: 14,
                                                height: 25 * domain.length,
                                                width: 130,
                                                axis: null,
                                                marks: [
                                                    Plot.frame({ fill: "white", stroke: "currentColor" }),
                                                    Plot.dot(data, {
                                                        y: "ngram",
                                                        fill: d => scales.color!(d.ngram),
                                                        r: 5,
                                                        frameAnchor: "left",
                                                        symbol: "square2",
                                                        dx: 7,
                                                    }),
                                                    Plot.text(data, {
                                                        y: "ngram",
                                                        text: "ngram",
                                                        frameAnchor: "left",
                                                        dx: 14,
                                                    }),
                                                    Plot.text(data, {
                                                        y: "ngram",
                                                        text: (data: any) => data.frequency.toExponential(3),
                                                        frameAnchor: "right",
                                                        dx: -2,
                                                    }),
                                                    Plot.text([data[0]], {
                                                        frameAnchor: "top-left",
                                                        dy: -13,
                                                        text: "date",
                                                    }),
                                                ],
                                            }),
                                        );
                                    }
                                    return g.node();
                                },
                            },
                        ),
                    ),
                ),
                Plot.ruleY([0]),
                Plot.ruleX([Date.UTC(...App.first_bucket)]),
            ],
        });
        this.chart.innerHTML = "";
        this.chart.append(plot);
    }

    set_error(error: string) {
        this.error.innerHTML = error;
        this.error.removeAttribute("class");
    }

    clear_error() {
        this.error.setAttribute("class", "invisible");
    }

    do_query() {
        this.timing.innerHTML = `Querying...`;
        http_get(
            `/query?q=${encodeURIComponent(this.query)}&ci=${this.case_insensitive}&combine=${this.combine}`,
            (res: string | Error) => {
                if (res instanceof Error) {
                    this.set_error(res.message);
                    return;
                }
                try {
                    const raw_data = JSON.parse(res) as encoded_query_response;
                    if (raw_data.error) {
                        this.set_error(raw_data.error);
                    } else {
                        this.clear_error();
                    }
                    this.render_chart(raw_data);
                } catch (e) {
                    this.set_error(`Internal error ${e}`);
                    console.log(e);
                }
            },
        );
    }
}

const app = new App();
