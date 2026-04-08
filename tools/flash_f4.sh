#!/usr/bin/env bash
#
# Flash script for STM32F4 Motor Control firmware
# Updated for new modular directory structure
#

set -euo pipefail

PROJECT="STM32F407_MotorControl"
HEX_FILE="build/f4/firmware/$PROJECT.hex"
CFG_FILE="tools/stm32f4.cfg"
TIMEOUT_SECONDS=30

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'  # No Color

# ============================================================================
# Pre-flight Checks
# ============================================================================

pre_flight_check() {
    local checks_failed=0
    
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}Pre-Flight Checks${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
    
    # Check 1: OpenOCD installed
    if command -v openocd >/dev/null 2>&1; then
        echo -e "${GREEN}✅${NC} OpenOCD found"
    else
        echo -e "${RED}❌${NC} OpenOCD NOT installed"
        echo "   Install: sudo apt install openocd (Ubuntu) or brew install openocd (Mac)"
        checks_failed=$((checks_failed + 1))
    fi
    
    # Check 2: Firmware file exists
    if [[ -f "$HEX_FILE" ]]; then
        HEX_SIZE=$(wc -c < "$HEX_FILE")
        echo -e "${GREEN}✅${NC} Firmware found ($(( HEX_SIZE / 1024 ))KB)"
    else
        echo -e "${RED}❌${NC} Firmware NOT found: $HEX_FILE"
        echo "   Run: tools/build_f4.sh rebuild"
        checks_failed=$((checks_failed + 1))
    fi
    
    # Check 3: Config file exists
    if [[ -f "$CFG_FILE" ]]; then
        echo -e "${GREEN}✅${NC} Config file found: $CFG_FILE"
    else
        echo -e "${RED}❌${NC} Config file NOT found: $CFG_FILE"
        checks_failed=$((checks_failed + 1))
    fi
    
    if [[ $checks_failed -gt 0 ]]; then
        echo -e "\n${RED}❌ $checks_failed check(s) failed. Cannot proceed.${NC}"
        return 1
    fi
    
    echo -e "\n${GREEN}✅ All checks passed!${NC}\n"
    return 0
}

# ============================================================================
# Flash Firmware
# ============================================================================

flash_firmware() {
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}Flashing STM32F407 Motor Control (F4)${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
    
    echo -e "${BLUE}Firmware: $HEX_FILE${NC}\n"
    
    # Create OpenOCD script with unlock + program
    cat > /tmp/flash_f4.tcl << OPENOCD_SCRIPT
# Connect to target
init

# Reset and halt
reset halt

# Unlock STM32F4 flash
stm32f4x unlock 0

# Program firmware (erase + write)
program "$HEX_FILE" verify

# Reset to run
reset run

# Done
exit
OPENOCD_SCRIPT
    
    echo -e "${BLUE}Erasing and programming...${NC}"
    
    # Flash with timeout
    timeout $TIMEOUT_SECONDS openocd -f "$CFG_FILE" -f /tmp/flash_f4.tcl 2>&1 || {
        echo -e "${RED}❌ Flashing failed or timed out${NC}"
        return 1
    }
    
    echo -e "\n${GREEN}✅ F4 Motor Control flash complete!${NC}"
    return 0
}

# ============================================================================
# MAIN
# ============================================================================

main() {
    if ! pre_flight_check; then
        exit 1
    fi
    
    if ! flash_firmware; then
        exit 1
    fi
    
    exit 0
}

main "$@"
