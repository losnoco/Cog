#!/bin/bash
set -euo pipefail

OUT_DIR="${SRCROOT}/Generated"
OUT_FILE="${OUT_DIR}/Secrets.swift"
mkdir -p "${OUT_DIR}"

# API_KEY comes from xcconfig and is available to build scripts as an env var
LASTFM_API_KEY="${LASTFM_API_KEY:-}"
LASTFM_API_SECRET="${LASTFM_API_SECRET:-}"
LASTFM_SESSION="${LASTFM_SESSION:-}"

replacer() {
    while read line; do
        line=$( echo $line | sed 's/"/\\"/g' )
        eval echo $line
    done
}

replacer > "${OUT_FILE}" <<EOF
// Auto-generated. Do not edit by hand.
public enum Secrets {
    public static let lastFmApiKey: String = "${LASTFM_API_KEY}"
    public static let lastFmApiSecret: String = "${LASTFM_API_SECRET}"
    public static let lastFmSession: String = "${LASTFM_SESSION}"
}
EOF