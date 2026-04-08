#!/usr/bin/env bash
#
# STM32F103C8T6 LED Test - Flash Script
# Usage: bash flash.sh
#

set -euo pipefail

# Get project root
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$PROJECT_ROOT"

# Check if we're in right place
if [[ ! -f "stm32f1.cfg" ]]; then
    echo "❌ ERROR: Cannot find stm32f1.cfg"
    exit 1
fi

# ============================================================================
# Configuration
# ============================================================================

PROJECT="F103_LED_Test"
HEX_FILE="build/test_led/firmware/$PROJECT.hex"
CFG_FILE="stm32f1.cfg"

# ============================================================================
# Colors
# ============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

# ============================================================================
# Pre-flight checks
# ============================================================================

echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${CYAN}Pre-Flight Checks${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"

# Check OpenOCD
if ! command -v openocd &> /dev/null; then
    echo -e "${RED}❌ OpenOCD not found${NC}"
    echo "   Install: apt-get install openocd (Linux) or brew install openocd (macOS)"
    exit 1
fi
echo -e "${GREEN}✅${NC} OpenOCD found: $(openocd --version | head -1)"

# Check firmware file
if [[ ! -f "$HEX_FILE" ]]; then
    echo -e "${RED}❌ Firmware not found: $HEX_FILE${NC}"
    echo "   Run: bash tests/test_led/build.sh rebuild"
    exit 1
fi
FW_SIZE=$(wc -c < "$HEX_FILE")
echo -e "${GREEN}✅${NC} Firmware found: $HEX_FILE ($(( FW_SIZE / 1024 ))KB)"

# Check config file
if [[ ! -f "$CFG_FILE" ]]; then
    echo -e "${RED}❌ Config file not found: $CFG_FILE${NC}"
    exit 1
fi
echo -e "${GREEN}✅${NC} Config file: $CFG_FILE"

echo ""

# ============================================================================
# Flash with OpenOCD
# ============================================================================

echo -e "${CYAN}Flashing STM32F103C8T6...${NC}\n"

openocd \
    -f "$CFG_FILE" \
    -c "init" \
    -c "reset halt" \
    -c "flash write_image erase $HEX_FILE" \
    -c "verify_image $HEX_FILE" \
    -c "reset run" \
    -c "shutdown" \
    2>&1 | grep -v "^Info"

if [[ ${PIPESTATUS[0]} -eq 0 ]]; then
    echo -e "\n${GREEN}✅ Flash successful!${NC}"
    echo "   LED test should be running on your board now"
else
    echo -e "\n${RED}❌ Flash failed!${NC}"
    exit 1
fi
