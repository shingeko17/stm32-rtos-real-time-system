#!/usr/bin/env bash
set -euo pipefail

PROJECT="STM32F407_MotorControl"
HEX_FILE="build/firmware/$PROJECT.hex"
CFG_FILE="stm32f4.cfg"
TIMEOUT_SECONDS=30

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'  # No Color

# Spinner frames
FRAMES=('⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠇' '⠏')

# ============================================================================
# Pre-flight Checks (Integrated Diagnostics)
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
        echo "   Run: ./build.sh rebuild"
        checks_failed=$((checks_failed + 1))
    fi
    
    # Check 3: Config file exists
    if [[ -f "$CFG_FILE" ]]; then
        echo -e "${GREEN}✅${NC} Config file found: $CFG_FILE"
    else
        echo -e "${RED}❌${NC} Config file NOT found: $CFG_FILE"
        checks_failed=$((checks_failed + 1))
    fi
    
    # Check 4: Test OpenOCD connection
    if [[ $checks_failed -eq 0 ]]; then
        echo -e "\n${CYAN}Testing OpenOCD connection...${NC}"
        # Create temp script
        TEMP_OUT=$(mktemp)
        
        # Run with timeout
        if timeout 5 openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
            -c "init" -c "targets" -c "shutdown" > "$TEMP_OUT" 2>&1; then
            
            if grep -q "stm32f4x.cpu" "$TEMP_OUT"; then
                echo -e "${GREEN}✅${NC} ST-Link detected and connected to target"
            else
                echo -e "${RED}❌${NC} OpenOCD failed to connect to target"
                echo -e "   ${YELLOW}Output:${NC}"
                grep -i "error\|failed\|unable" "$TEMP_OUT" | head -3 | sed 's/^/      /'
                checks_failed=$((checks_failed + 1))
            fi
        else
            echo -e "${RED}❌${NC} OpenOCD connection test failed/timeout"
            echo -e "   ${YELLOW}Check:${NC}"
            echo "   1. ST-Link USB cable connected?"
            echo "   2. Board powered ON?"
            echo "   3. Try: power cycle board (unplug 30s, replug)"
            checks_failed=$((checks_failed + 1))
        fi
        
        rm -f "$TEMP_OUT"
    fi
    
    # Summary
    echo ""
    if [[ $checks_failed -eq 0 ]]; then
        echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${GREEN}✅ All checks passed! Ready to flash.${NC}"
        echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
        return 0
    else
        echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${RED}❌ $checks_failed check(s) failed. Fix above before flashing.${NC}"
        echo -e "${RED}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
        return 1
    fi
}

# Run pre-flight checks
pre_flight_check || exit 1

# ============================================================================
# Progress Spinner Function
# ============================================================================
show_progress() {
    local duration=$1
    local start_time=$(date +%s)
    local frame_index=0
    
    while true; do
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        
        # Check if process still running (parent bash process)
        if ! kill -0 $FLASH_PID 2>/dev/null; then
            break
        fi
        
        # Check timeout
        if [[ $elapsed -ge $duration ]]; then
            echo -e "\r${RED}[TIMEOUT]${NC} Flashing exceeded ${duration}s limit"
            kill $FLASH_PID 2>/dev/null || true
            return 1
        fi
        
        # Display spinner
        local frame=${FRAMES[$((frame_index % ${#FRAMES[@]}))]}
        printf "\r${BLUE}${frame}${NC} Processing... (${elapsed}s/${duration}s)"
        frame_index=$((frame_index + 1))
        
        sleep 0.1
    done
}

# ============================================================================
# Main Flash Process
# ============================================================================
echo -e "${YELLOW}🔄 Starting firmware flash...${NC}"
echo -e "${BLUE}Firmware:${NC} $HEX_FILE"
echo -e "${BLUE}Config:${NC}   $CFG_FILE"
echo ""

# Run flash in background to show progress
timeout $TIMEOUT_SECONDS openocd -f "$CFG_FILE" -c "program \"$HEX_FILE\" verify reset exit" > /tmp/flash_output.log 2>&1 &
FLASH_PID=$!

# Show progress spinner
if show_progress $TIMEOUT_SECONDS; then
    FLASH_EXIT_CODE=0
else
    FLASH_EXIT_CODE=$?
fi

# Wait for process to finish
wait $FLASH_PID 2>/dev/null || FLASH_EXIT_CODE=$?

# ============================================================================
# Status Output
# ============================================================================
if [[ $FLASH_EXIT_CODE -eq 0 ]]; then
    echo -e "\r${GREEN}✅ Successful${NC}                    "
    echo -e "${GREEN}[SUCCESS]${NC} Firmware flashed and verified successfully!"
    exit 0
else
    echo -e "\r${RED}❌ Failed${NC}                        "
    # Show last few lines of error log
    echo -e "${RED}[FAILED]${NC} Flash operation failed (Exit code: $FLASH_EXIT_CODE)"
    echo -e "\n${YELLOW}Last log lines:${NC}"
    tail -n 5 /tmp/flash_output.log 2>/dev/null || echo "No log available"
    exit 1
fi
