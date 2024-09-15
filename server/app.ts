import express from "express";
import webpack from "webpack";
import webpackDevMiddlware from "webpack-dev-middleware";
import { is_string } from "./util.js";

const app = express();
const port = 3000;

import sqlite3 from "sqlite3";
import { query_response, query_result } from "./schema.js";
const db = new sqlite3.Database("test.db3", sqlite3.OPEN_READONLY);

function formulate_query(part: string, case_insensitive: boolean) {
    if (case_insensitive) {
        return [
            `
            SELECT
                LOWER(ngrams.ngram) AS ngram, frequencies.months_since_epoch, SUM(frequencies.frequency) AS frequency
            FROM frequencies
            INNER JOIN ngrams ON ngrams.ngram_id = frequencies.ngram_id
            WHERE LOWER(ngrams.ngram) = ?
            GROUP BY LOWER(ngrams.ngram), frequencies.months_since_epoch;
            `,
            part.toLowerCase(),
        ] as [string, string];
    } else {
        return [
            `
            SELECT
                ngrams.ngram, frequencies.months_since_epoch, frequencies.frequency
            FROM frequencies
            INNER JOIN ngrams ON ngrams.ngram_id = frequencies.ngram_id
            WHERE ngrams.ngram = ?;
            `,
            part,
        ] as [string, string];
    }
}

function do_query(part: string, case_insensitive: boolean): Promise<query_result> {
    return new Promise<query_result>((resolve, reject) => {
        const data: query_result = {};
        db.each(
            ...formulate_query(part, case_insensitive),
            (err, row: { ngram: string; months_since_epoch: number; frequency: number }) => {
                if (err) {
                    reject(err);
                }
                if (!(row.ngram in data)) {
                    data[row.ngram] = [];
                }
                data[row.ngram].push({
                    year_month: Date.UTC(2017, row.months_since_epoch),
                    frequency: row.frequency,
                });
            },
            () => {
                resolve(data);
            },
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
        handle_query(raw_query, case_insensitive).then((data: query_response) => {
            res.setHeader("Content-Type", "application/json");
            res.end(JSON.stringify(data));
        }).catch(() => {
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
