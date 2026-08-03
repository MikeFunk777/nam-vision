#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
README_PATH="$REPO_ROOT/README.md"
OUTPUT_PDF="${1:-$REPO_ROOT/NeuralAmpModeler/manual/NAM Division README.pdf}"
TEMP_DIR=$(mktemp -d)
HTML_PATH="$TEMP_DIR/readme.html"
RENDERED_PDF="$TEMP_DIR/NAM Division README.pdf"
CHROME_PID=""

cleanup()
{
  if [ -n "$CHROME_PID" ] && kill -0 "$CHROME_PID" 2>/dev/null; then
    kill "$CHROME_PID" 2>/dev/null || true
    wait "$CHROME_PID" 2>/dev/null || true
  fi

  if [ -d "$TEMP_DIR" ]; then
    find "$TEMP_DIR" -depth -delete
  fi
}
trap cleanup EXIT

if [ ! -f "$README_PATH" ]; then
  echo "ERROR: README not found at $README_PATH" >&2
  exit 1
fi

CHROME="${CHROME_BIN:-}"
if [ -z "$CHROME" ] && [ -x "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" ]; then
  CHROME="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
fi

if [ -z "$CHROME" ]; then
  for candidate in google-chrome chromium chromium-browser; do
    if command -v "$candidate" >/dev/null 2>&1; then
      CHROME=$(command -v "$candidate")
      break
    fi
  done
fi

if [ -z "$CHROME" ] || [ ! -x "$CHROME" ]; then
  echo "ERROR: Google Chrome or Chromium is required to render the README PDF." >&2
  exit 1
fi

ruby - "$README_PATH" "$HTML_PATH" "$REPO_ROOT" <<'RUBY'
require "cgi"
require "rdoc"
require "rdoc/markdown"
require "rdoc/markup/to_html"
require "rdoc/options"
require "uri"

readme_path, html_path, repo_root = ARGV
markdown = File.read(readme_path, encoding: "UTF-8")

# Avoid freezing a transient CI result into the installer documentation.
markdown.gsub!(/^\[!\[Build\]\([^\n]+\)\]\([^\n]+\)\n+/, "")

# RDoc escapes inline HTML, so normalize README image tags back to Markdown.
markdown.gsub!(/<img\b[^>]*>/i) do |tag|
  source = tag[/\bsrc="([^"]+)"/i, 1]
  alt = tag[/\balt="([^"]*)"/i, 1] || ""
  source ? "![#{alt}](#{source})" : tag
end

# Repository-root image paths need to be relative when rendered from a local file.
markdown.gsub!(/\]\(\/docs\//, "](docs/")

document = RDoc::Markdown.parse(markdown)
body = RDoc::Markup::ToHtml.new(RDoc::Options.new).convert(document)
body.gsub!(%r{<span><a href="[^"]+">&para;</a> <a href="#top">&uarr;</a></span>}, "")

base_url = URI::DEFAULT_PARSER.escape("file://#{repo_root}/")
html = <<~HTML
  <!doctype html>
  <html lang="en">
  <head>
    <meta charset="utf-8">
    <base href="#{CGI.escapeHTML(base_url)}">
    <title>NAM Division README</title>
    <style>
      @page { size: A4; margin: 16mm 15mm 18mm; }
      * { box-sizing: border-box; }
      html { color: #24292f; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; font-size: 10.5pt; line-height: 1.5; }
      body { margin: 0 auto; max-width: 760px; }
      h1, h2, h3 { color: #1f2328; line-height: 1.25; page-break-after: avoid; }
      h1 { border-bottom: 1px solid #d0d7de; font-size: 27pt; margin: 0 0 18px; padding-bottom: 8px; }
      h2 { border-bottom: 1px solid #d8dee4; font-size: 19pt; margin: 28px 0 12px; padding-bottom: 5px; }
      h3 { font-size: 14pt; margin: 22px 0 8px; }
      p { margin: 0 0 12px; orphans: 3; widows: 3; }
      a { color: #0969da; text-decoration: none; }
      blockquote { border-left: 4px solid #d0d7de; color: #57606a; margin: 0 0 16px; padding: 4px 16px; }
      blockquote p:last-child { margin-bottom: 0; }
      pre { background: #f6f8fa; border: 1px solid #d8dee4; border-radius: 6px; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 8.5pt; line-height: 1.4; overflow-wrap: anywhere; padding: 12px; page-break-inside: avoid; white-space: pre-wrap; }
      code { background: #eff1f3; border-radius: 4px; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 90%; padding: 0.15em 0.35em; }
      pre code, pre span { background: transparent; padding: 0; }
      img { display: block; height: auto; margin: 14px auto 20px; max-width: 100%; page-break-inside: avoid; }
      hr { border: 0; border-top: 1px solid #d0d7de; margin: 24px 0; }
      strong { color: #1f2328; }
    </style>
  </head>
  <body>
  #{body}
  </body>
  </html>
HTML

File.write(html_path, html, mode: "w", encoding: "UTF-8")
RUBY

mkdir -p "$(dirname "$OUTPUT_PDF")"
CHROME_LOG="$TEMP_DIR/chrome.log"
"$CHROME" \
  --headless \
  --disable-background-networking \
  --disable-component-update \
  --disable-extensions \
  --disable-gpu \
  --hide-scrollbars \
  --allow-file-access-from-files \
  --no-default-browser-check \
  --no-first-run \
  --no-pdf-header-footer \
  --user-data-dir="$TEMP_DIR/chrome" \
  --virtual-time-budget=5000 \
  "--print-to-pdf=$RENDERED_PDF" \
  "file://$HTML_PATH" >"$CHROME_LOG" 2>&1 &
CHROME_PID=$!

last_size=0
stable_checks=0
for _ in $(seq 1 240); do
  if [ -s "$RENDERED_PDF" ]; then
    current_size=$(wc -c < "$RENDERED_PDF" | tr -d ' ')
    if [ "$current_size" = "$last_size" ]; then
      stable_checks=$((stable_checks + 1))
      if [ "$stable_checks" -ge 4 ]; then
        break
      fi
    else
      stable_checks=0
      last_size=$current_size
    fi
  elif ! kill -0 "$CHROME_PID" 2>/dev/null; then
    wait "$CHROME_PID" 2>/dev/null || true
    CHROME_PID=""
    cat "$CHROME_LOG" >&2
    echo "ERROR: Chrome exited before creating the README PDF." >&2
    exit 1
  fi

  sleep 0.25
done

if [ ! -s "$RENDERED_PDF" ] || [ "$stable_checks" -lt 4 ]; then
  cat "$CHROME_LOG" >&2
  echo "ERROR: Timed out while rendering the README PDF." >&2
  exit 1
fi

if kill -0 "$CHROME_PID" 2>/dev/null; then
  kill "$CHROME_PID" 2>/dev/null || true
fi
wait "$CHROME_PID" 2>/dev/null || true
CHROME_PID=""

mv "$RENDERED_PDF" "$OUTPUT_PDF"

if [ ! -s "$OUTPUT_PDF" ]; then
  echo "ERROR: README PDF was not created." >&2
  exit 1
fi

echo "Wrote $OUTPUT_PDF"
