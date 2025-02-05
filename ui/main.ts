import "./style.scss";

import * as d3 from "d3";
import * as Plot from "@observablehq/plot";
import moment from "moment";

import { encoded_query_response, query_result } from "../shared/schema";
import { debounce, http_get, round_down_exponential } from "./utils";
import { first_bucket, last_bucket, maybe_slash } from "../shared/common";
import { occlusionY } from "./occlusion";

class App {
    query_input: HTMLInputElement;
    case_insensitive_button: HTMLElement;
    combine_button: HTMLElement;
    smooth_button: HTMLElement;
    error: HTMLElement;
    chart: HTMLElement;
    timing: HTMLElement;

    query: string;
    case_insensitive = false;
    combine = false;
    smooth = true;

    last_query_res: encoded_query_response = {
        series: [],
        time: 0,
    };

    constructor() {
        this.query_input = document.getElementById("query")! as HTMLInputElement;
        this.query = this.query_input.value;
        this.query_input.addEventListener("keyup", debounce(this.query_keystroke.bind(this), 500), false);
        this.case_insensitive_button = document.getElementById("case-insensitive")!;
        this.case_insensitive_button.addEventListener("click", this.case_insensitive_button_press.bind(this), false);
        this.combine_button = document.getElementById("combine-series")!;
        this.combine_button.addEventListener("click", this.combine_button_press.bind(this), false);
        this.smooth_button = document.getElementById("smooth")!;
        this.smooth_button.addEventListener("click", this.smooth_button_press.bind(this), false);
        this.error = document.getElementById("error")!;
        this.chart = document.getElementById("chart")!;
        this.timing = document.getElementById("timing")!;

        this.set_button_states();

        this.do_query();
    }

    case_insensitive_button_press() {
        this.case_insensitive = !this.case_insensitive;
        this.set_button_states();
        this.do_query();
    }

    combine_button_press() {
        this.combine = !this.combine;
        this.set_button_states();
        this.do_query();
    }

    smooth_button_press() {
        this.smooth = !this.smooth;
        this.set_button_states();
        this.render_chart();
    }

    set_button_states() {
        this.case_insensitive_button.setAttribute("class", this.case_insensitive ? "on" : "");
        this.combine_button.setAttribute("class", this.combine ? "on" : "");
        this.smooth_button.setAttribute("class", this.smooth ? "on" : "");
    }

    query_keystroke() {
        this.query = this.query_input.value;
        this.do_query();
    }

    static months_after_first_bucket(months: number) {
        return Date.UTC(first_bucket[0], first_bucket[1] + months);
    }

    prepare_data(response: encoded_query_response) {
        // we get an array of results for each part of the query
        // we want to consolidate down to a single dict, keeping the order (we use that for the color domain)
        this.timing.innerHTML = `Query time: ${response.time} ms`;
        const consolidated_result: query_result = new Map(response.series.flat(1));
        const last_bucket_ts = new Date(...last_bucket).getTime();
        // fill in gaps
        for (const series of consolidated_result.values()) {
            series.sort((a, b) => a.year_month - b.year_month);
            // there should be an entry for each moth from first bucket to last bucket, fill in gaps with 0's
            for (let month = 0; App.months_after_first_bucket(month) <= last_bucket_ts; month++) {
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

    render_chart() {
        const [domain, data] = this.prepare_data(this.last_query_res);
        const windowed = (() => {
            const { channels } = (
                Plot.dotY(
                    data,
                    Plot.windowY(
                        { k: this.smooth ? 5 : 1, anchor: "middle", strict: false },
                        {
                            x: "date",
                            y: "frequency",
                            stroke: "ngram",
                            z: "ngram",
                        },
                    ),
                ) as Plot.Dot & { initialize: () => { channels: any } }
            ).initialize();
            return (i: number) => ({
                date: channels.x.value[i],
                frequency: channels.y.value[i],
                ngram: channels.stroke.value[i],
            });
        })();
        const plot = Plot.plot({
            grid: true,
            width: 1500,
            height: 800,
            x: {
                domain: [new Date(...first_bucket), new Date(...last_bucket)],
            },
            y: data.length > 0 ? {} : { domain: [0, 0.1] },
            style: { fontSize: "16px", overflow: "visible" },
            marginLeft: 50,
            marginBottom: 50,
            marginRight: 100,
            color: {
                legend: true,
                className: "legend-text",
                domain: domain.length === 0 ? [""] : domain,
            } as Plot.ScaleOptions,
            marks: [
                // the series
                Plot.lineY(
                    data,
                    Plot.windowY(
                        { k: this.smooth ? 5 : 1, anchor: "middle", strict: false },
                        {
                            x: "date",
                            y: "frequency",
                            stroke: "ngram",
                            z: "ngram",
                            curve: "catmull-rom",
                        },
                    ),
                ),
                // format the y ticks
                Plot.axisY({
                    label: "Frequency",
                    marginBottom: 10,
                    tickFormat: value => round_down_exponential(value, 0),
                }),
                // labels at the end
                Plot.text(
                    data,
                    occlusionY(
                        { minDistance: 20 },
                        Plot.selectLast(
                            Plot.windowY(
                                { k: this.smooth ? 5 : 1, anchor: "middle", strict: false },
                                {
                                    x: "date",
                                    y: "frequency",
                                    z: "ngram",
                                    text: "ngram",
                                    fill: "ngram",
                                    textAnchor: "start",
                                    dx: 3,
                                },
                            ),
                        ),
                    ),
                ),
                // cursor
                Plot.ruleX(
                    data,
                    Plot.pointerX(
                        Plot.binX({}, { x: "date", thresholds: 10000, stroke: "rgb(185, 185, 185)", strokeWidth: 2 }),
                    ),
                ),
                // tooltip
                // https://github.com/observablehq/plot/issues/2003
                // https://github.com/observablehq/plot/discussions/2209
                Plot.tip(
                    data,
                    Plot.pointerX(
                        Plot.binX(
                            {
                                y: () => 0,
                                title: (v: any) => Array.from(v, windowed),
                            },
                            {
                                title: (_: any, i: any) => i,
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
                                    // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
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
                                                        dx: 3,
                                                        text: (data: any) => moment.utc(data.date).format("MMMM YYYY"),
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
                // axis lines
                Plot.ruleY([0]),
                Plot.ruleX([Date.UTC(...first_bucket)]),
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
        const endpoint = `${maybe_slash(import.meta.env.BASE_URL)}query`;
        http_get(
            `${endpoint}?q=${encodeURIComponent(this.query)}&ci=${this.case_insensitive}&combine=${this.combine}`,
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
                    this.last_query_res = raw_data;
                    this.render_chart();
                } catch (e) {
                    this.set_error(`Internal error ${e}`);
                    console.log(e);
                }
            },
        );
    }
}

const app = new App();
