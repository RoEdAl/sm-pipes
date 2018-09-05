/*
    babel-preset-RelWithDebInfo.js
*/

const preset = {
    plugins: [
        "@babel/plugin-proposal-do-expressions",
        ["conditional-compile", {
            "define": {
                "CONFIG": "RelWithDebInfo"
            }
        }],
        "minify-dead-code-elimination",
        "transform-merge-sibling-variables"
    ]
};

module.exports = () => preset;
