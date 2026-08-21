import histogram from "./histogram.js";
import * as d3 from "d3";
import csv from "./assets/unemployment-x.csv";

d3.csv(csv).then((data, error) => { 
// d3.csv("https://static.observableusercontent.com/files/8a6057f29caa4e010854bfc31984511e074ff9042ec2a99f30924984821414fbaeb75e59654e9303db359dfa0c1052534691dac86017c4c2f992d23b874f9b6e?response-content-disposition=attachment%3Bfilename*%3DUTF-8%27%27unemployment-x.csv").then((data, error) => { 
  if (error) {
    console.log(error);
  } else {
    // console.log(data);
    histogram(data, {
      value: d => d.rate,
      label: "Unemployment rate (%) →",
      width: 500,
      height: 500,
      color: "steelblue"
    });
  };
}); 