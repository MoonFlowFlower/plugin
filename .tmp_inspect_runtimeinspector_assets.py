import unreal

panel = unreal.load_asset('/RuntimeInspector/UI/WBP_InspectorPanel')
row = unreal.load_asset('/RuntimeInspector/UI/WBP_PropertyRow')
for name, obj in [('panel', panel), ('row', row)]:
    print('ASSET', name, obj.get_path_name() if obj else None, obj.get_class().get_name() if obj else None)
    if not obj or not hasattr(obj, 'widget_tree'):
        continue
    tree = obj.widget_tree
    widgets = tree.get_all_widgets()
    for w in widgets:
        wn = w.get_name()
        if wn == 'LV_Properties' or wn.startswith('LV_'):
            print('  WIDGET', wn, w.get_class().get_name())
            for prop in ['entry_widget_class', 'entry_spacing', 'wheel_scroll_multiplier', 'allow_focus_on_interaction']:
                if hasattr(w, prop):
                    try:
                        print('   ', prop, getattr(w, prop))
                    except Exception as e:
                        print('   ', prop, 'ERR', e)
        if wn == 'CanvasPanel_0' or wn == 'VerticalBox_0':
            print('  root-ish', wn, w.get_class().get_name())

# class / interface info
try:
    print('ROW_INTERFACES', [i.get_name() for i in row.get_class().get_interfaces()])
except Exception as e:
    print('ROW_INTERFACES_ERR', e)
