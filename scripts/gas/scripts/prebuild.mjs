import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import dotenv from "dotenv";
import os from "os";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repo_root = path.resolve(__dirname, "..");
const clasp_path = path.join(repo_root, ".clasp.json");

const argv_list = process.argv.slice(2);
let force_flag = false;
let env_arg;

for (let index = 0; index < argv_list.length; index += 1) {
  const token = argv_list[index];
  if (token === "--force" || token === "-f") {
    force_flag = true;
    continue;
  }
  if (token.startsWith("--env=")) {
    env_arg = token.slice("--env=".length);
    continue;
  }
  if (token === "--env" || token === "-e") {
    env_arg = argv_list[index + 1];
    index += 1;
  }
}

const env_path = resolve_env_path(env_arg, repo_root);
const env_result = dotenv.config({ path: env_path });
const env_values = env_result.parsed ?? {};

const script_id = read_env_value("CLASP_SCRIPT_ID", env_values);
if (script_id === undefined) {
  console.error("Missing env: CLASP_SCRIPT_ID");
  process.exit(1);
}

const clasp_config = { scriptId: script_id };
const root_dir_value = read_env_value("CLASP_ROOT_DIR", env_values);
if (root_dir_value !== undefined) {
  clasp_config.rootDir = root_dir_value;
}
const project_id_value = read_env_value("CLASP_PROJECT_ID", env_values);
if (project_id_value !== undefined) {
  clasp_config.projectId = project_id_value;
}

if (fs.existsSync(clasp_path) && !force_flag) {
  console.log(".clasp.json exists. Skipping.");
  process.exit(0);
}

fs.writeFileSync(clasp_path, JSON.stringify(clasp_config, null, 2) + "\n");
console.log(force_flag ? "Regenerated .clasp.json" : "Generated .clasp.json");

function resolve_env_path(raw_path, base_dir) {
  const candidate = normalise_path_token(raw_path ?? "");
  if (!candidate) return path.resolve(base_dir, ".env");
  const expanded = expand_home(candidate);
  if (path.isAbsolute(expanded)) return expanded;
  return path.resolve(base_dir, expanded);
}

function read_env_value(key, env_store) {
  if (has_own(env_store, key)) return env_store[key];
  if (has_own(process.env, key)) return process.env[key];
  return undefined;
}

function expand_home(candidate) {
  return candidate.replace(/^~(?=($|[\\/]))/, os.homedir());
}

function normalise_path_token(value) {
  return value && value.trim ? value.trim() : value;
}

function has_own(object, key) {
  return Object.prototype.hasOwnProperty.call(object, key);
}
