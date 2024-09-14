const path = require("path");
const HtmlWebpackPlugin = require("html-webpack-plugin");

ENTRY_PATH = path.resolve(__dirname, "ui/main.ts");
DIST_PATH = path.resolve(__dirname, "build/dist");

module.exports = {
    mode: "development",
    entry: {
        index: ENTRY_PATH,
    },
    output: {
        path: DIST_PATH,
        filename: "[name].[contenthash].js",
        clean: true,
        publicPath: "/",
    },
    module: {
        rules: [
            {
                test: /\.html$/,
                use: ["html-loader"],
            },
            {
                test: /\.css$/,
                use: ["style-loader", "css-loader"],
            },
            {
                test: /\.tsx?$/,
                use: "ts-loader",
                exclude: /node_modules/,
            },
            {
                test: /\.s[ac]ss$/,
                use: ["style-loader", "css-loader", "sass-loader"],
            },
        ],
    },
    resolve: {
        extensions: [".tsx", ".ts", ".js"],
    },
    plugins: [
        new HtmlWebpackPlugin({
            // filename: path.resolve(__dirname, "ui/index.html"),
            // filename: "index.html",
            template: path.resolve(__dirname, "ui/index.html"),
        }),
    ],
    devtool: "inline-source-map",
    devServer: {
        static: DIST_PATH,
        hot: true,
    },
};
