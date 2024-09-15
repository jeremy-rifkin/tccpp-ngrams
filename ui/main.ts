require("./style.scss");

import * as d3 from "d3";
import * as Plot from "@observablehq/plot";

import { query_response } from "../server/schema.js";
import { debounce, http_get } from "./utils";
import { occlusionY } from "./occlusion";

class App {
    static readonly first_bucket: [number, number] = [2017, 6]; // july 2017
    query_input: HTMLInputElement;
    case_insensitive_button: HTMLElement;
    chart: HTMLElement;

    query: string;
    case_insensitive = false;

    constructor() {
        this.query_input = document.getElementById("query")! as HTMLInputElement;
        this.query = this.query_input.value;
        this.query_input.addEventListener("keyup", debounce(this.query_keystroke.bind(this), 500), false);
        this.case_insensitive_button = document.getElementById("case-insensitive")!;
        this.case_insensitive_button.addEventListener("click", this.case_insensitive_button_press.bind(this), false);
        this.chart = document.getElementById("chart")!;
        this.do_query();
    }

    case_insensitive_button_press() {
        this.case_insensitive = !this.case_insensitive;
        this.case_insensitive_button.setAttribute("class", this.case_insensitive ? "on" : "");
        this.do_query();
    }

    query_keystroke() {
        this.query = this.query_input.value;
        this.do_query();
    }

    static months_after_first_bucket(months: number) {
        return Date.UTC(this.first_bucket[0], this.first_bucket[1] + months);
    }

    prepare_data(raw_data: query_response) {
        // fill in gaps
        for (const series of Object.values(raw_data)) {
            series.sort((a, b) => a.year_month - b.year_month);
            for (let i = 0; i < series.length; i++) {
                if (series[i].year_month != App.months_after_first_bucket(i)) {
                    series.splice(i, 0, { year_month: App.months_after_first_bucket(i), frequency: 0 });
                }
            }
        }
        // pivot
        const data = Object.entries(raw_data)
            .map(([ngram, entries]) => {
                return entries.map(({ year_month, frequency }) => {
                    return { date: new Date(year_month), frequency, ngram };
                });
            })
            .flat();
        return data;
    }

    render_chart(raw_data: query_response) {
        const data = this.prepare_data(raw_data);
        const plot = Plot.plot({
            grid: true,
            width: 1500,
            height: 800,
            style: { fontSize: "16px" },
            marginLeft: 50,
            marginBottom: 50,
            marginRight: 100,
            y: {
                label: "Frequency",
                tickFormat: value => value.toExponential(0),
            },
            marks: [
                Plot.lineY(data, {
                    x: "date",
                    y: "frequency",
                    stroke: "ngram",
                    z: "ngram",
                }),
                Plot.text(
                    data,
                    occlusionY(
                        { minDistance: 20 },
                        Plot.selectLast({
                            x: "date",
                            y: "frequency",
                            z: "ngram",
                            text: "ngram",
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
                                                height: 25 * Object.keys(raw_data).length,
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

    do_query() {
        http_get(`/query?q=${encodeURIComponent(this.query)}&ci=${this.case_insensitive}`, (res: string) => {
            const raw_data = JSON.parse(res) as query_response;
            this.render_chart(raw_data);
        });
    }
}

const app = new App();
