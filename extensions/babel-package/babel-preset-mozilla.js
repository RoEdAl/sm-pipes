/*
    babel-preset-Mozilla.js
*/

const preset = {
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

module.exports = () => preset;
