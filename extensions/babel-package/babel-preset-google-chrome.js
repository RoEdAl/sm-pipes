/*
    babel-preset-Chrome.js
*/

const preset = {
    plugins: [
        ["conditional-compile", {
            "define": {
                "BROWSER": "Chrome"
            }
        }]
    ],
    presets: [
        ["@babel/preset-env", {
            "targets": {
                "chrome": "68"
            },
            "modules": false,
            "spec": true
        }]
    ]
};

module.exports = () => preset;
