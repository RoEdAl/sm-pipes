/*
    babel-preset-Release.js
*/

const preset = {
    plugins: [
        "@babel/plugin-proposal-do-expressions",
        ["conditional-compile", {
            "define": {
                "CONFIG": "Release"
            }
        }],
        "transform-remove-console",
        "minify-dead-code-elimination",
        "transform-merge-sibling-variables"
    ]
};

module.exports = () => preset;