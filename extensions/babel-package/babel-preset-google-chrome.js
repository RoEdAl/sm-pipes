/*
    babel-preset-Chrome.js
*/

module.exports = () => {
    return {
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
};