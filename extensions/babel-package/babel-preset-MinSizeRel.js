/*
    babel-preset-MinSizeRel.js
*/

module.exports = () => {
    return {
        plugins: [
            "@babel/plugin-proposal-do-expressions",
            ["conditional-compile", {
                "define": {
                    "CONFIG": "MinSizeRel"
                }
            }],
            "transform-remove-console",
            "minify-dead-code-elimination",
            "transform-merge-sibling-variables"
        ]
    };
};