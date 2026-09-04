from pathlib import Path

root = Path('.')
path = root / 'src/gameLayer/multyPlayer/tick.cpp'
text = path.read_text(encoding='utf-8')
needle = '#include <multyPlayer/actionResync.h>\n'
replacement = needle + '#include <multyPlayer/serverActionValidation.h>\n'
if text.count(needle) != 1:
    raise SystemExit(f'expected exactly one actionResync include, found {text.count(needle)}')
if '#include <multyPlayer/serverActionValidation.h>' in text:
    raise SystemExit('serverActionValidation include already present')
path.write_text(text.replace(needle, replacement, 1), encoding='utf-8')

(root / '.github/scripts/apply-item-use-validation-include.py').unlink()
(root / '.github/workflows/apply-item-use-validation-include.yml').unlink()
