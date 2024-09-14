import express from "express";
import webpack from "webpack";
import webpackDevMiddlware from "webpack-dev-middleware";
import { is_string } from "./util.js";

const app = express();
const port = 3000;

import sqlite3 from "sqlite3";
import { query_response } from "./schema.js";
const db = new sqlite3.Database("test.db3", sqlite3.OPEN_READONLY);

app.get("/query", (req, res) => {
    const raw_query = req.query.q;
    if (!is_string(raw_query)) {
        res.status(500);
        res.end();
    } else {
        // res.send(query);
        // res.end();
        // const statement = db.prepare("SELECT grams.ngram, frequencies.months_since_epoch, frequencies.ngram from frequencies JOIN grams ON grams.ngram_id = frequencies.ngram_id WHERE grams.ngram == ? LIMIT 100;");
        // statement.run(query, (...args: any[]) => {
        //     console.log(args);
        // });
        const query = raw_query.split(",").map(q => q.trim());
        const data: query_response = {};
        db.each(
            `SELECT ngrams.ngram, frequencies.months_since_epoch, frequencies.frequency from frequencies JOIN ngrams ON ngrams.ngram_id = frequencies.ngram_id WHERE ngrams.ngram IN (${Array(query.length).fill("?")});`,
            query,
            (err, row: { ngram: string; months_since_epoch: number; frequency: number }) => {
                if (err) {
                    throw err;
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
                // console.log(data);
                res.setHeader("Content-Type", "application/json");
                res.end(JSON.stringify(data));
            },
        );
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
