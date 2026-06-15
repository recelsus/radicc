import esbuild from "esbuild";
import { GasPlugin } from "esbuild-gas-plugin";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import dotenv from "dotenv";
import os from "os";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const root_dir = path.resolve(__dirname, "..");
const src_dir = path.join(root_dir, "src");

const env_result = dotenv.config({ path: path.join(root_dir, ".env") });
const env_values = env_result.parsed ?? {};
const dist_dir = resolve_dist_dir(read_env_value("CLASP_ROOT_DIR", env_values), root_dir);

const src_manifest_path = path.join(src_dir, "appsscript.json");
const dist_manifest_path = path.join(dist_dir, "appsscript.json");

ensure_dir(src_dir);
clean_dist_dir(dist_dir);
ensure_dir(dist_dir);

await esbuild.build({
  entryPoints: [path.join(src_dir, "main.ts")],
  outfile: path.join(dist_dir, "main.js"),
  bundle: true,
  minify: true,
  platform: "browser",
  format: "iife",
  plugins: [GasPlugin, copy_manifest_plugin()],
});

console.log("Build complete");

function copy_manifest_plugin() {
  return {
    name: "copy-manifest",
    setup(build) {
      build.onEnd(() => {
        if (!fs.existsSync(src_manifest_path)) {
          console.log("src/appsscript.json not found. Skipping manifest copy.");
          return;
        }
        ensure_dir(dist_dir);
        fs.copyFileSync(src_manifest_path, dist_manifest_path);
        console.log("Copied src/appsscript.json to dist");
      });
    },
  };
}

function ensure_dir(target_dir) {
  if (!fs.existsSync(target_dir)) {
    fs.mkdirSync(target_dir, { recursive: true });
  }
}

function clean_dist_dir(target_dir) {
  fs.rmSync(target_dir, { recursive: true, force: true });
}

function resolve_dist_dir(raw_value, base_dir) {
  const candidate = normalise_path_token(raw_value ?? "");
  if (!candidate) return path.join(base_dir, "dist");
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
