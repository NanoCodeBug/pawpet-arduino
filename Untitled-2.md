TODO:
validate clock toggle rate is 30hz and fix counters as needed 
need phillips screws for the back
need more heat set inserts
need more 470/480 ohm 805 resistors



https://forum.arduino.cc/t/samd21-power-consumption-too-high-in-active-mode/668966/2


Generic clock generator 0               8 division factor bits - DIV[7:0]
Generic clock generator 1               16 division factor bits - DIV[15:0]
Generic clock generators 2              5 division factor bits - DIV[4:0]
Generic clock generators 3 - 8          8 division factor bits - DIV[7:0]

rtc will be on anyways for time keeping

add BOD33 disable in bootloader flashing scripts?

condition where NVM RWS being 1 in bootloader will prevent startup below 2.1v
which is in theory near the end of useful battery life anyways

add ability for poweroff requiring push button reset?
from system menu and for when battery is low
- how to do this? disable all clocks?


display says it operates down 2.7v
- max frame frequency of 65hz?
- claims EXTCOMIN should be below framerate frequency, does not indicate why
- extcom freq of 54-65hz

3) A still image should be displayed less than two hours, if it is necessary to display still image longer than two 
hour, display image data must be refreshed in order to avoid sticking image on LCD panel

Memory LCDs do not generate this signal internally. It must be supplied using one of two methods: 
software, or external clock...
When the software clock is selected [EXTMODE = L], bit V of the command bit string sets the state of VCOM. 
See Figure 2. This bit must toggle (by writing to the panel) at least once per second. 

Power Scenario 1 
1hz write 1hz refresh
Here all of the pixels will be written to the panel once per second. For the 1.35-inch panel, the quiescent 
current for the panel is 2.1μA. Each write to the panel (assuming that the entire panel is written), takes 
360μA; and lasts for about 5.38ms (assuming a 2 MHz SPI clock speed). 
If averaged over a second, this amounts to an “average” currant draw of approximately 2μa. Therefore, the 
panel would have an average power draw of 20.5μW (5V operation). Note that a VCOM toggle can be done 
whenever the panel is written to, including a data write. 

Power Scenario 2 
30 sec write, 1hz refresh
Here, we write all the pixels to the panel only every 30 seconds. The current required to write to the panel 
“averaged” over that time is approximately 0.1μA. If VCOM is toggled once a second (using the Change 
VCOM command), the additional power draw for this action (assuming that it is done 29 times) is an average 
of 0.1μA. Therefore the average power draw over the 30-second period becomes 11.5μW. See Figure 10.
Applying these conditions to the 2.7-inch panel: writing all pixels to the whole panel once a second takes an 
average of 485μW. Writing all pixels once every 30 seconds (with a 1-second VCOM toggle) takes 135μW.




    uint32_t userWord0 = *((uint32_t *)NVMCTRL_USER);       // Read fuses for user word 0
    uint32_t userWord1 = *((uint32_t *)(NVMCTRL_USER + 4)); // Read fuses for user word 1
    NVMCTRL->CTRLB.bit.CACHEDIS = 1;                        // Disable the cache
    NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2;           // Set the address
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_EAR |            // Erase the auxiliary user page row
                         NVMCTRL_CTRLA_CMDEX_KEY;
    while (!NVMCTRL->INTFLAG.bit.READY)             // Wait for the NVM command to complete
        NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK; // Clear the error flags
    NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2;   // Set the address
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_PBC |    // Clear the page buffer
                         NVMCTRL_CTRLA_CMDEX_KEY;
    while (!NVMCTRL->INTFLAG.bit.READY)                            // Wait for the NVM command to complete
        NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;                // Clear the error flags
    *((uint32_t *)NVMCTRL_USER) = userWord0 & ~FUSES_BOD33_EN_Msk; // Disable the BOD33 enable fuse in user word 0
    *((uint32_t *)(NVMCTRL_USER + 4)) = userWord1;                 // Copy back user word 1 unchanged
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_WAP |                   // Write to the user page
                         NVMCTRL_CTRLA_CMDEX_KEY;
    while (!NVMCTRL->INTFLAG.bit.READY)             // Wait for the NVM command to complete
        NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK; // Clear the error flags
    NVMCTRL->CTRLB.bit.CACHEDIS = 0;                // Enable the cache


pos 8, 5 bits
pos 15, 2 bits
pos 14, 1 bit
