/*
    babel-preset-Debug.js
*/

module.exports = () => {
    return {
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
};