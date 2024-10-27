import moment from "moment";
import chalk from "chalk";

export function is_string(value: string | unknown): value is string {
    return typeof value === "string" || value instanceof String;
}

export class M {
    static get_timestamp() {
        return moment().format("YYYY-MM-DD HH:mm:ss");
    }
    static log(...args: any[]) {
        process.stdout.write(`[${M.get_timestamp()}] [log]   `);
        console.log(...args);
    }
    static debug(...args: any[]) {
        process.stdout.write(`${chalk.gray(`[${M.get_timestamp()}] [debug]`)} `);
        console.log(...args);
    }
    static info(...args: any[]) {
        process.stdout.write(`${chalk.blueBright(`[${M.get_timestamp()}] [info] `)} `);
        console.log(...args);
    }
    static warn(...args: any[]) {
        process.stdout.write(`${chalk.yellowBright(`[${M.get_timestamp()}] [warn] `)} `);
        console.log(...args);
    }
    static error(...args: any[]) {
        process.stdout.write(`${chalk.redBright(`[${M.get_timestamp()}] [error]`)} `);
        console.log(...args);
        console.trace();
    }
}
