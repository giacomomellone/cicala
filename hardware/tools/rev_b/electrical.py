"""Check selected Rev B parts and the charge veto against the 103JT R/T table."""
import json
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'pcb'))
import ksexp as k
from audit_temperature import audit as temperature

# Semitec JT datasheet, 103JT resistance table in ohms (0–60 °C).
JT_RT=((0,27700),(10,18070),(20,12110),(25,10000),(30,8301),(40,5811),(50,4147),(60,3011))


def audit(netlist, board):
    data=k.load(netlist);pins={}
    for net in k.children(k.child(data,'nets'),'net'):
        for node in k.children(net,'node'):
            pins[k.child(node,'ref')[1],k.child(node,'pin')[1]]=k.child(net,'name')[1]
    expected={'J3':{'1':'VBAT_CELL','2':'BATT_NTC','3':'GND'},
              'U2':{'1':'VSYS','2':'VBAT','5':'GND','6':'BATT_NTC','9':'CHG_STAT1','10':'USB_VBUS','11':'GND'},
              'U3':{'1':'VSYS','2':'GND','3':'GND','6':'3V3','8':'GND','10':'VSYS'},
              'J2':{'5':'EPD_VSH2','9':'EPD_BUSY','10':'PNL_RESET','11':'PNL_DC','12':'PNL_CS','13':'PNL_CLK','14':'PNL_MOSI'}}
    for ref,ps in expected.items():
        for pin,net in ps.items():
            if pins.get((ref,pin))!=net:raise ValueError(f'{ref}.{pin}: expected {net}, got {pins.get((ref,pin))}')
    for pin in ('1','4'):
        if not pins.get(('J2',pin),'').startswith('unconnected-'):raise ValueError(f'J2.{pin} must remain unconnected')
    if not(pins['U3','4']==pins['R14','2']==pins['R15','1']):raise ValueError('Feedback divider connectivity changed')
    fps={k.ref_of(f):f for f in k.children(k.load(board),'footprint')}
    chosen={'U1':'ESP32-S3-WROOM-1-N16','J2':'503480-2400','SW1':'B3F-4050','SW2':'B3F-4050'}
    for ref,mpn in chosen.items():
        props={p[1]:p[2]for p in k.children(fps[ref],'property')}
        if props.get('MPN')!=mpn:raise ValueError(f'{ref}: selected MPN changed')
    values={k.child(c,'ref')[1]:k.child(c,'value')[1]for c in k.children(k.child(data,'components'),'comp')}
    if values['R8']not in {'3k','3.00k','3.0k'}:raise ValueError('Expected 100 mA charger programming resistor')
    result=temperature(netlist,rt=JT_RT)
    result['sensor']='Semitec 103JT-025; published JT table, conservative ±5% R/T allowance'
    result['physical_tests']='Bonded sensor lag and actual cell temperature remain first-article measurements'
    return result


if __name__=='__main__':
    result=audit(sys.argv[1],sys.argv[2]);Path(sys.argv[3]).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
