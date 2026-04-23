#!/usr/bin/env python3
"""
Generate src/html/index.html by assembling modular CSS, HTML, and JS files.

Run manually:
    python tool/gen_index.py

Or automatically via extra_script_fs.py build hook.
"""
import os

BASE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'src', 'html')


def read(relpath):
    with open(os.path.join(BASE_DIR, relpath), 'r', encoding='utf-8') as f:
        return f.read()


CSS_FILES = [
    'css/base.css',
    'css/layout.css',
    'css/cards.css',
    'css/settings.css',
]

LAYOUT_FILE = 'components/layout/layout.html'

PAGE_FILES = [
    'components/settings/settings.html',
    'components/logs/logs.html',
]

JS_FILES = [
    'js/navigation.js',
    'components/settings/settings.js',
]

TEMPLATE = """\
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>EPD Weather Config</title>
<style>
{css}
</style>
</head>
<body>
<div class="layout-wrapper">

{layout}

{pages}

</div><!-- /layout-main -->
</div><!-- /layout-main-container -->
</div><!-- /layout-wrapper -->

<div id="toast-container" class="toast-container"></div>

<script>
{js}
</script>
</body>
</html>
"""


def build():
    css    = '\n'.join(read(f) for f in CSS_FILES)
    layout = read(LAYOUT_FILE)
    pages  = '\n'.join(read(f) for f in PAGE_FILES)
    js     = '\n'.join(read(f) for f in JS_FILES)

    html = TEMPLATE.format(css=css, layout=layout, pages=pages, js=js)

    out_path = os.path.join(BASE_DIR, 'index.html')
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write(html)
    print(f'[gen_index] index.html written → {out_path} ({len(html):,} bytes)')


if __name__ == '__main__':
    build()
