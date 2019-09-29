let webpack = require('webpack');
let HtmlWebPackPlugin = require('html-webpack-plugin');

module.exports = {
    module: {
        rules: [
            {
                test: /\.js$/,
                exclude: /node_modules/,
                use: {
                    loader: "babel-loader"
                }
            },
            {
                test: /\.css/,
                loaders: ['style-loader', 'css-loader'],
            },
            {
                test: /\.html$/,
                use: [
                    {
                        loader: "html-loader",
                        options: {
                            minimize: false
                        }
                    }
                ]
            },
            {
                test:/\.(png)$/,
                use: [
                    {
                        loader: "file-loader",
                        options: {}
                    }
                ]
            }
        ]
    },
    entry: './src/index.js',
    output: {
        path: __dirname + '/dist',
        filename:'index_bundle.js'
    },
    plugins: [
        new HtmlWebPackPlugin({
            title: "Seadrone Command Center",
            template: "./src/index.html",
            filename: "./index.html"
        })
    ]
};