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
    ]
  };
};