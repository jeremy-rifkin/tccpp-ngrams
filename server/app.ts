import express from "express";
import webpack from "webpack";
import webpackDevMiddlware from "webpack-dev-middleware";
import { is_string } from "./util.js";

const app = express();
const port = 3000;

import duckdb from "duckdb";
import { encoded_query_response, encoded_query_result, query_response, query_result } from "./schema.js";
const db = new duckdb.Database("ngrams.duckdb");
const con = db.connect();

function tokenize(part: string) {
    return part.split(/(\s+)/).filter(e => e.trim().length > 0);
}

function formulate_query(part: string[], case_insensitive: boolean): [query: string, ...params: (string | number)[]] {
    const column_names = [...part.map((_, i) => `gram_${i}`)];
    const conditions = column_names.map(column =>
        case_insensitive
            ? `LOWER(ngrams_${column_names.length}.${column}) GLOB ?`
            : `ngrams_${column_names.length}.${column} GLOB ?`,
    );
    return [
        `
        WITH top_ngrams AS (
            SELECT ngram_id, ${column_names.join(", ")}
            FROM ngrams_${column_names.length}
            WHERE
                ${conditions.join(" AND ")}
            ORDER BY total DESC
            LIMIT 10
        )
        SELECT
            ${column_names.map(column => `top_ngrams.${column}`).join(", ")}, frequencies.months_since_epoch, frequencies.frequency
        FROM frequencies
        INNER JOIN top_ngrams ON top_ngrams.ngram_id = frequencies.ngram_id
        ORDER BY ${column_names.map(column => `top_ngrams.${column}`).join(", ")}, frequencies.months_since_epoch;
        `,
        ...part.map(s => (case_insensitive ? s.toLocaleLowerCase() : s)),
    ];
}

function do_query(part: string, case_insensitive: boolean): Promise<query_result> {
    return new Promise<query_result>((resolve, reject) => {
        const tokenized_part = tokenize(part);
        if (tokenized_part.length > 5) {
            reject("Too many grams");
        }
        const data: query_result = new Map();
        const [query, ...params] = formulate_query(tokenized_part, case_insensitive);
        // console.log(query);
        con.all(
            query,
            ...params,
            (err, res) => {
                if (err) {
                    reject(err);
                }
                for(const row of res) {
                    const ngram = [...Array(tokenized_part.length).keys()].map(i => row[`gram_${i}`]).join(" ");
                    if (!data.has(ngram)) {
                        data.set(ngram, []);
                    }
                    data.get(ngram)!.push({
                        year_month: Date.UTC(2017, row.months_since_epoch),
                        frequency: row.frequency,
                    });
                }
                resolve(data);
            }
        );
    });
}

async function handle_query(raw_query: string, case_insensitive: boolean): Promise<query_response> {
    const parts = raw_query.split(",").map(q => q.trim());
    const data = await Promise.all(parts.map(part => do_query(part, case_insensitive)));
    return data;
}

app.get("/query", (req, res) => {
    const raw_query = req.query.q;
    const case_insensitive = req.query.ci === "true";
    if (!is_string(raw_query)) {
        res.status(500);
        res.end();
    } else {
        handle_query(raw_query, case_insensitive)
            .then((data: query_response) => {
                res.setHeader("Content-Type", "application/json");
                res.end(JSON.stringify(data.map(result => Array.from(result)) as encoded_query_response));
            })
            .catch(() => {
                res.status(500);
                res.end();
            });
    }
});

async function setup_webpack_dev_middlware(router: express.Router) {
    const config = (await import("../webpack.config.cjs")).default;
    console.log(config);
    const compiler = webpack(config as any);
    app.use(
        webpackDevMiddlware(compiler, {
            publicPath: config.output.publicPath,
        }),
    );
    app.listen(port, () => {
        console.log(`[server]: Server is running at http://localhost:${port}`);
    });
}

setup_webpack_dev_middlware(app);
