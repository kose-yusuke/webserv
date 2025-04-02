#!/bin/bash
echo "=== Running Redirect Tests ==="

declare -a urls=(
  "http://localhost:8080/redirect301"
  "http://localhost:8080/redirect302"
  "http://localhost:8080/redirect303"
  "http://localhost:8080/redirect307"
  "http://localhost:8080/redirect308"
  "http://localhost:8888/redir"
  "http://localhost:8888/redir/override"
)

for url in "${urls[@]}"; do
  echo -e "--- $url"
  curl -i -s "$url"
done

echo "=== Redirect Tests Completed ==="
