# Rev A board export

`kveld_rev_a_board.step` is generated from the KiCad constraint board for case interference checks. It contains the board outline and holes, not placed production components. Regenerate it after changing the board geometry and run `just hw-pcb-check`.
