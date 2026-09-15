"""Audit the selected B3F-4050 top-view hole pattern and logical terminal pairs."""
import ksexp as k


def check(board, contract):
    buttons=contract['buttons']
    for index,ref in enumerate(('SW1','SW2')):
        fp=next(f for f in k.children(board,'footprint') if k.ref_of(f)==ref)
        if fp[1]!='cicala:SW_Omron_B3F-4050' or 'through_hole' not in k.child(fp,'attr'):
            raise ValueError(f'{ref}: expected selected through-hole switch footprint')
        actual=[]
        for pad in k.children(fp,'pad'):
            xy=tuple(map(float,k.child(pad,'at')[1:3]))
            drill=float(k.child(pad,'drill')[1])
            net=k.child(pad,'net')
            actual.append((pad[1],pad[2],xy,drill,net[1] if net else ''))
        signal='BTN_CATEGORY' if index==0 else 'BTN_NEXT'
        expected=[(str(pin),'thru_hole',(x,y),1.2,signal if pin==1 else 'GND')
                  for x in (-6.25,6.25) for pin,y in ((1,-2.5),(2,2.5))]
        expected += [('', 'np_thru_hole',(0.,y),1.8,'') for y in (-4.5,4.5)]
        if sorted(actual)!=sorted(expected):raise ValueError(f'{ref}: switch holes or terminal-pair nets differ from drawing')
        at=list(map(float,k.child(fp,'at')[1:]))
        if at[:2]!=buttons['centres'][index] or (len(at)>2 and at[2]!=0):
            raise ValueError(f'{ref}: switch center or orientation changed')
