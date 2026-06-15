import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { execFileSync } from "child_process";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repo_root = path.resolve(__dirname, "..");
const clasp_path = path.join(repo_root, ".clasp.json");
const src_dir = path.join(repo_root, "src");
const src_manifest_path = path.join(src_dir, "appsscript.json");

if (!fs.existsSync(clasp_path)) {
  console.log(".clasp.json not found. Skipping manifest sync.");
  process.exit(0);
}

const clasp_config = JSON.parse(fs.readFileSync(clasp_path, "utf8"));
const clasp_root_dir = typeof clasp_config.rootDir === "string" && clasp_config.rootDir.trim()
  ? clasp_config.rootDir.trim()
  : ".";
const resolved_clasp_root = path.resolve(repo_root, clasp_root_dir);

try {
  execFileSync("npx", ["clasp", "pull"], { cwd: repo_root, stdio: "inherit" });
} catch (error) {
  console.error(`Failed to pull manifest via clasp: ${error.message ?? error}`);
  process.exit(error.status ?? 1);
}

const pulled_manifest_path = path.join(resolved_clasp_root, "appsscript.json");
if (!fs.existsSync(pulled_manifest_path)) {
  console.log("Manifest not found in clasp rootDir. Skipping copy.");
  process.exit(0);
}

fs.mkdirSync(src_dir, { recursive: true });
fs.copyFileSync(pulled_manifest_path, src_manifest_path);
console.log("Copied manifest into src/appsscript.json");
