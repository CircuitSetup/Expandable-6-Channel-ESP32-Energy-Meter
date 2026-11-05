const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const source = fs.readFileSync(path.join(__dirname, "../src/data/config.js"), "utf8");
const saveCal = source.slice(source.indexOf("  self.saveCal = function ()"), source.indexOf("$(function ()"));
const alerts = [];
let posts = 0;
const self = { config: {}, saveCalFetching() {}, saveCalSuccess() {} };
for (let i = 1; i <= 42; i++) {
  Object.assign(self.config, {
    [`ct${i}_cal`]: () => "27961",
    [`cur${i}_mul`]: () => "-1.2",
    [`pow${i}_mul`]: () => "1",
    [`ct${i}_name`]: () => `CT${i}`,
    [`gain${i}_cal`]: () => "1",
  });
}
Object.assign(self.config, {
  voltage_cal: () => "7305",
  voltage2_cal: () => "7305",
  freq_cal: () => "60",
});
const context = vm.createContext({
  self,
  baseEndpoint: "",
  alert: message => alerts.push(message),
  $: { post() { posts++; return { fail: () => ({ always() {} }) }; } },
});
const isNumber = source.slice(source.indexOf("function isNumber("), source.indexOf("function BaseViewModel"));
vm.runInContext(isNumber + saveCal.slice(0, saveCal.lastIndexOf("}")), context);
for (const invalid of [".", "--", "1.2.3", "1-", "Infinity", ""]) {
  self.config.cur1_mul = () => invalid;
  self.saveCal();
}
assert.equal(alerts.length, 6);
assert.equal(posts, 0);
self.config.cur1_mul = () => "-1.2";
self.config.ct1_cal = () => "65536";
self.saveCal();
assert.equal(alerts.length, 7);
assert.equal(posts, 0);
self.config.ct1_cal = () => "65535";
self.saveCal();
assert.equal(posts, 1);
