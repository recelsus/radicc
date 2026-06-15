import fs from "fs";
import path from "path";
import dotenv from "dotenv";
import os from "os";

const DEFAULT_ENV_PATH = ".env";
const DEFAULT_TEMPLATE_PATH = path.join("src", "env.ts.in");
const DEFAULT_OUTPUT_PATH = path.join("src", "env.ts");
const ENV_ALIASES = {
  RADICC_SERVER_URL: ["RADICC_API_URL"],
};
const OPTIONAL_ENV_DEFAULTS = {
  RADICC_SERVER_API_KEY: "",
  GOOGLE_DRIVE_FOLDER_ID: "",
};

const args = process.argv.slice(2);
const env_path = resolve_env_path(args[0] ?? DEFAULT_ENV_PATH, process.cwd());
const template_path = resolve_path(args[1] ?? DEFAULT_TEMPLATE_PATH);
const output_path = resolve_path(args[2] ?? DEFAULT_OUTPUT_PATH);

if (!fs.existsSync(template_path)) {
  console.log(`Template not found (${template_path}). Skipping env.ts generation.`);
  process.exit(0);
}

const template_source = fs.readFileSync(template_path, "utf8");
const placeholder_re = /\{\{([A-Z0-9_]+)\}\}/g;
const placeholder_keys = new Set();
let match;
while ((match = placeholder_re.exec(template_source)) !== null) {
  placeholder_keys.add(match[1]);
}

const env_contents = fs.existsSync(env_path) ? fs.readFileSync(env_path, "utf8") : "";
const env_values = env_contents ? dotenv.parse(env_contents) : {};
const resolved_keys = new Map();

for (const key of placeholder_keys) {
  const resolved = resolve_env_value(key, env_values);
  if (resolved !== undefined) {
    resolved_keys.set(key, resolved);
    continue;
  }
  if (has_own(OPTIONAL_ENV_DEFAULTS, key)) {
    resolved_keys.set(key, OPTIONAL_ENV_DEFAULTS[key]);
    continue;
  }
  console.error(`Missing env key: ${key}`);
  process.exit(1);
}

const replaced = template_source.replace(
  placeholder_re,
  (_full, key) => JSON.stringify(resolved_keys.get(key) ?? "")
);

fs.mkdirSync(path.dirname(output_path), { recursive: true });
fs.writeFileSync(output_path, replaced);
console.log(`Generated ${path.relative(process.cwd(), output_path)}`);

function resolve_env_path(raw_path, base_dir) {
  const candidate = normalise_path_token(raw_path ?? "");
  if (!candidate) return path.resolve(base_dir, DEFAULT_ENV_PATH);
  const expanded = expand_home(candidate);
  if (path.isAbsolute(expanded)) return expanded;
  return path.resolve(base_dir, expanded);
}

function resolve_path(raw_path) {
  const candidate = normalise_path_token(raw_path ?? "");
  const expanded = expand_home(candidate);
  if (path.isAbsolute(expanded)) return expanded;
  return path.resolve(process.cwd(), expanded);
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

function resolve_env_value(key, env_store) {
  const candidates = [key, ...(ENV_ALIASES[key] ?? [])];
  for (const candidate of candidates) {
    if (has_own(env_store, candidate)) return env_store[candidate];
    if (has_own(process.env, candidate)) return process.env[candidate];
  }
  return undefined;
}
