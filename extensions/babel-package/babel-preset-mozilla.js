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
    ]
  };
};