#!/bin/bash
# scripts/generate-assets.sh
# Generate all PNG assets from SVG templates
# Usage: bash scripts/generate-assets.sh
set -e

REPO="$(cd "$(dirname "$0")/.." && pwd)"
SVG_DIR="$REPO/assets/svg"
ASSETS="$REPO/assets"

echo "=== AnyClaw LVGL Asset Generator ==="
echo "Repo: $REPO"

# ── Helper: render SVG → PNG at multiple sizes ──
render_svg() {
    local svg_in="$1"; shift
    local base_name="$1"; shift
    local out_dir="$1"; shift
    local sizes=("$@")

    mkdir -p "$out_dir"
    for sz in "${sizes[@]}"; do
        local out="$out_dir/${base_name}_${sz}.png"
        rsvg-convert -w "$sz" -h "$sz" "$svg_in" -o "$out"
        pngquant --quality=85-95 --speed=1 --force --output "$out" "$out" 2>/dev/null || true
        optipng -o1 -quiet "$out" 2>/dev/null || true
        echo "  ✓ $out ($(stat -c%s "$out") bytes)"
    done
}

# ── Helper: render single SVG → single PNG ──
render_one() {
    local svg_in="$1"
    local out="$2"
    local w="$3"
    local h="${4:-$w}"

    mkdir -p "$(dirname "$out")"
    rsvg-convert -w "$w" -h "$h" "$svg_in" -o "$out"
    pngquant --quality=85-95 --speed=1 --force --output "$out" "$out" 2>/dev/null || true
    optipng -o1 -quiet "$out" 2>/dev/null || true
    echo "  ✓ $out ($(stat -c%s "$out") bytes)"
}

# ══════════════════════════════════════════════
# 1. GARLIC MASCOT (3 themes × 4 sizes + body/sprout)
# ══════════════════════════════════════════════
echo ""
echo "── 1. Garlic Mascot ──"

# Theme color maps
declare -A STEM=( [matcha]="#3DD68C" [peachy]="#FF7F50" [mochi]="#A67B5B" )
declare -A STEM_HI=( [matcha]="#6EE7B7" [peachy]="#FFB347" [mochi]="#C9A96E" )
declare -A BODY_START=( [matcha]="#FFFFFF" [peachy]="#FFFFFF" [mochi]="#FFFDF9" )
declare -A BODY_END=( [matcha]="#E8F5E9" [peachy]="#FFF0E6" [mochi]="#F5EDE4" )
declare -A STROKE=( [matcha]="#B2DFDB" [peachy]="#FFD4B8" [mochi]="#D4C4B0" )
declare -A BLUSH=( [matcha]="#FFB3B3" [peachy]="#FF9F7A" [mochi]="#C4868C" )

for theme in matcha peachy mochi; do
    echo "  Theme: $theme"

    # Generate themed SVG (full mascot)
    themed_svg="/tmp/garlic_${theme}.svg"
    sed -e "s|__STEM__|${STEM[$theme]}|g" \
        -e "s|__STEM_HI__|${STEM_HI[$theme]}|g" \
        -e "s|__BODY_START__|${BODY_START[$theme]}|g" \
        -e "s|__BODY_END__|${BODY_END[$theme]}|g" \
        -e "s|__STROKE__|${STROKE[$theme]}|g" \
        -e "s|__BLUSH__|${BLUSH[$theme]}|g" \
        "$SVG_DIR/mascot/garlic_base.svg" > "$themed_svg"

    # Render at 4 sizes → icons/ai/
    for sz in 24 32 48 96; do
        render_one "$themed_svg" "$ASSETS/icons/ai/garlic_${sz}_${theme}.png" "$sz"
    done

    # Render body (garlic_48) → mascot/{theme}/
    render_one "$themed_svg" "$ASSETS/mascot/${theme}/garlic_48.png" 48

    # Generate themed sprout SVG
    sprout_svg="/tmp/garlic_sprout_${theme}.svg"
    sed -e "s|__STEM__|${STEM[$theme]}|g" \
        -e "s|__STEM_HI__|${STEM_HI[$theme]}|g" \
        "$SVG_DIR/mascot/garlic_sprout_base.svg" > "$sprout_svg"

    # Render sprout at 512px → mascot/{theme}/
    render_one "$sprout_svg" "$ASSETS/mascot/${theme}/garlic_sprout.png" 512
done

# Copy matcha as fallback
echo "  Copying matcha as fallback..."
cp "$ASSETS/icons/ai/garlic_24_matcha.png" "$ASSETS/icons/ai/garlic_24.png"
cp "$ASSETS/icons/ai/garlic_32_matcha.png" "$ASSETS/garlic_32.png"
cp "$ASSETS/icons/ai/garlic_48_matcha.png" "$ASSETS/garlic_48.png"
cp "$ASSETS/mascot/matcha/garlic_sprout.png" "$ASSETS/garlic_sprout.png"
echo "  ✓ Fallback files updated"

# ══════════════════════════════════════════════
# 2. LOBSTER AI AVATAR
# ══════════════════════════════════════════════
echo ""
echo "── 2. Lobster Avatar ──"

for sz in 24 32 48; do
    render_one "$SVG_DIR/mascot/lobster_base.svg" "$ASSETS/icons/ai/lobster_01_${sz}.png" "$sz"
done

# ══════════════════════════════════════════════
# 3. TRAY ICONS (5 themes × 4 states × 4 sizes)
# ══════════════════════════════════════════════
echo ""
echo "── 3. Tray Icons ──"

# Theme body colors (garlic silhouette tinted per theme)
declare -A TRAY_BODY=(
    [matcha]="#B2DFDB"
    [peachy]="#FFD4B8"
    [mochi]="#D4C4B0"
    [classic]="#808080"
    [light]="#D1D5DB"
)

# State LED colors per theme
declare -A LED_WHITE_MATCHA="#E8ECF4" LED_GREEN_MATCHA="#3DD68C" LED_RED_MATCHA="#FF6B6B" LED_YELLOW_MATCHA="#FFBE3D"
declare -A LED_WHITE_PEACHY="#8B7355" LED_GREEN_PEACHY="#FF7F50" LED_RED_PEACHY="#FF5C5C" LED_YELLOW_PEACHY="#FFB347"
declare -A LED_WHITE_MOCHI="#B0A394" LED_GREEN_MOCHI="#A67B5B" LED_RED_MOCHI="#C47070" LED_YELLOW_MOCHI="#C9A96E"
declare -A LED_WHITE_CLASSIC="#808080" LED_GREEN_CLASSIC="#4EC9B0" LED_RED_CLASSIC="#F44747" LED_YELLOW_CLASSIC="#FFD700"
declare -A LED_WHITE_LIGHT="#9CA3AF" LED_GREEN_LIGHT="#10B981" LED_RED_LIGHT="#EF4444" LED_YELLOW_LIGHT="#F59E0B"

for theme in matcha peachy mochi classic light; do
    echo "  Theme: $theme"
    mkdir -p "$ASSETS/tray/${theme}"

    body_color="${TRAY_BODY[$theme]}"

    for state in white green red yellow; do
        led_var="LED_${state^^}_${theme^^}"
        led_color="${!led_var}"

        # Generate themed tray SVG
        tray_svg="/tmp/tray_${theme}_${state}.svg"
        sed -e "s|__BODY_COLOR__|${body_color}|g" \
            -e "s|__LED_COLOR__|${led_color}|g" \
            "$SVG_DIR/tray/tray_base.svg" > "$tray_svg"

        for sz in 16 20 32 48; do
            render_one "$tray_svg" "$ASSETS/tray/${theme}/tray_${state}_${sz}.png" "$sz"
        done
    done
done

# ══════════════════════════════════════════════
# 4. GENERATION SCRIPT SELF-REFERENCE
# ══════════════════════════════════════════════
echo ""
echo "=== Asset generation complete ==="
echo "Generated files:"
find "$ASSETS" -name "*.png" -newer "$SVG_DIR/mascot/garlic_base.svg" | wc -l
echo "PNG files created/updated"
