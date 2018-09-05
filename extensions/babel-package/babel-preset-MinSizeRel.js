/*
    babel-preset-MinSizeRel.js
*/

const preset = {
    plugins: [
        "@babel/plugin-proposal-do-expressions",
        ["conditional-compile", {
            "define": {
                "CONFIG": "MinSizeRel"
            }
        }],
        "transform-remove-console",
        "minify-dead-code-elimination",
        ["minify-mangle-names", {
            "topLevel": true
        }]
    ]
};

module.exports = () => preset;
