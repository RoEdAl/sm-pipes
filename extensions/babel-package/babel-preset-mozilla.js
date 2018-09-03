/*
    babel-preset-Mozilla.js
*/

module.exports = () => {
    return {
        plugins: [
            ["conditional-compile", {
                "define": {
                    "BROWSER": "Mozilla"
                }
            }]
        ],
        presets: [
            ["@babel/preset-env", {
                "targets": {
                    "firefox": "54"
                },
                "modules": false,
                "spec": true
            }]
        ]
    };
};