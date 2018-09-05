/*
    babel-preset-Debug.js
*/

const preset = {
    plugins: [
        "@babel/plugin-proposal-do-expressions",
        ["conditional-compile", {
            "define": {
                "CONFIG": "Debug"
            }
        }],
        "transform-merge-sibling-variables"
    ]
};

module.exports = () => preset;
