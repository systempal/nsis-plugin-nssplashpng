"""
Fix nsSplashPNG repo:
1. Rimuove dal tracking i file di build MSBuild in src/ (artifacts + DLL grezze)
2. Sposta le DLL pre-built in dist/ (struttura canonica)
3. Aggiunge regole .gitignore per escludere le cartelle Release in src/
4. Aggiunge dist/ al tracking (con DLL binarie)
5. Commit + push Gitea + push GitHub
"""
import os, sys, shutil, subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# --- Load .env ---
env_path = REPO.parent.parent / '.env'  # Launchers/.env
if env_path.exists():
    for line in env_path.read_text(encoding='utf-8').splitlines():
        line = line.strip()
        if line and not line.startswith('#') and '=' in line:
            k, v = line.split('=', 1)
            os.environ.setdefault(k.strip(), v.strip())

GITEA_URL  = os.environ['GITEA_URL']
GITEA_USER = os.environ['GITEA_USER']
GITEA_TOK  = os.environ['GITEA_TOKEN']
GH_USER    = os.environ['GITHUB_USER']
GH_TOK     = os.environ['GITHUB_TOKEN']
REPO_NAME  = 'nsis-plugin-nssplashpng'

def git(*args):
    r = subprocess.run(['git', '-C', str(REPO)] + list(args), capture_output=True, text=True)
    if r.returncode != 0:
        print(f'[git] stderr: {r.stderr.strip()}')
    return r

# 1. Rimuovi dal tracking tutti i file di build in src/
print('[1] Rimuovo artifacts di build dal tracking...')
tracked_artifacts = [
    'src/nsSplashPNG/Release Unicode/nsSplashPNG.dll.recipe',
    'src/nsSplashPNG/Release Unicode/nsSplashPNG.iobj',
    'src/nsSplashPNG/Release Unicode/nsSplashPNG.ipdb',
    'src/nsSplashPNG/Release/nsSplashPNG.dll.recipe',
    'src/nsSplashPNG/Release/nsSplashPNG.iobj',
    'src/nsSplashPNG/Release/nsSplashPNG.ipdb',
    'src/nsSplashPNG/x64/Release Unicode/nsSplashPNG.dll.recipe',
    'src/nsSplashPNG/x64/Release Unicode/nsSplashPNG.iobj',
    'src/nsSplashPNG/x64/Release Unicode/nsSplashPNG.ipdb',
    'src/Release Unicode/nsSplashPNG.dll',
    'src/Release/nsSplashPNG.dll',
    'src/x64/Release Unicode/nsSplashPNG.dll',
]
for f in tracked_artifacts:
    r = git('rm', '--cached', f)
    if r.returncode == 0:
        print(f'  [rm cached] {f}')
    else:
        print(f'  [skip] {f} — not tracked or already removed')

# 2. Sposta DLL in dist/ (struttura canonica)
print('[2] Sposto DLL in dist/...')
moves = [
    ('src/Release/nsSplashPNG.dll',              'dist/x86-ansi/nsSplashPNG.dll'),
    ('src/Release Unicode/nsSplashPNG.dll',       'dist/x86-unicode/nsSplashPNG.dll'),
    ('src/x64/Release Unicode/nsSplashPNG.dll',   'dist/amd64-unicode/nsSplashPNG.dll'),
]
for src_rel, dst_rel in moves:
    src = REPO / Path(src_rel)
    dst = REPO / Path(dst_rel)
    dst.parent.mkdir(parents=True, exist_ok=True)
    if src.exists():
        shutil.copy2(src, dst)
        print(f'  [copied] {src_rel} → {dst_rel}')
    elif dst.exists():
        print(f'  [exists] {dst_rel} (src già rimosso)')
    else:
        print(f'  [warn] {src_rel} non trovato — DLL mancante')

# 3. Aggiungi .gitignore rules per le cartelle MSBuild in src/
print('[3] Aggiorno .gitignore...')
gi_path = REPO / '.gitignore'
gi_content = gi_path.read_text(encoding='utf-8')
extra = (
    '\n# MSBuild output directories in src/ (build artifacts, not source)\n'
    'src/**/Release/\n'
    'src/**/Release Unicode/\n'
    'src/**/x64/\n'
    '*.recipe\n'
    '*.iobj\n'
    '*.ipdb\n'
)
if 'MSBuild output directories' not in gi_content:
    gi_path.write_text(gi_content + extra, encoding='utf-8')
    print('  [ok] .gitignore aggiornato')
else:
    print('  [skip] .gitignore già aggiornato')

# 4. Stage dist/ e .gitignore
print('[4] Stage dist/ e .gitignore...')
git('add', 'dist/')
git('add', '.gitignore')

# Verifica status
r = git('status', '--short')
print(r.stdout.strip() or '(nothing staged)')

# 5. Commit
print('[5] Commit...')
r = git('commit', '-m',
    'fix: move pre-built DLLs to dist/, remove MSBuild artifacts from tracking\n\n'
    '- src/Release*/ and src/nsSplashPNG/Release*/ were committed by mistake\n'
    '  (MSBuild artifacts: .iobj, .ipdb, .recipe + raw DLL outputs)\n'
    '- DLLs moved to canonical dist/{x86-ansi,x86-unicode,amd64-unicode}/\n'
    '- .gitignore updated to exclude src/**/Release*/ going forward'
)
if r.returncode != 0:
    print(f'[FAIL] commit: {r.stderr}', file=sys.stderr); sys.exit(1)
print(f'[ok] {r.stdout.strip()}')

# 6. Push Gitea
print('[6] Push Gitea...')
gitea_url = f"{GITEA_URL.replace('://', f'://{GITEA_USER}:{GITEA_TOK}@')}/{GITEA_USER}/{REPO_NAME}.git"
r = subprocess.run(['git', '-C', str(REPO), 'push', gitea_url, 'main'],
                   capture_output=True, text=True)
if r.returncode != 0:
    print(f'[FAIL] Gitea: {r.stderr}', file=sys.stderr); sys.exit(1)
print(f'[ok] Gitea: {r.stderr.strip() or "up to date"}')

# 7. Push GitHub
print('[7] Push GitHub...')
gh_url = f'https://{GH_USER}:{GH_TOK}@github.com/{GH_USER}/{REPO_NAME}.git'
r = subprocess.run(['git', '-C', str(REPO), 'push', gh_url, 'main'],
                   capture_output=True, text=True)
if r.returncode != 0:
    print(f'[FAIL] GitHub: {r.stderr}', file=sys.stderr); sys.exit(1)
print(f'[ok] GitHub: {r.stderr.strip() or "up to date"}')

print('[DONE]')
