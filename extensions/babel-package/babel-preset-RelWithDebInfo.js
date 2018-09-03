/*
    babel-preset-RelWithDebInfo.js
*/

module.exports = () => {
    return {
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
};