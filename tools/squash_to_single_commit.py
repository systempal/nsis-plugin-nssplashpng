"""
Squash everything in the nsSplashPNG repo into a single initial commit,
then force-push to Gitea and GitHub.

Steps:
1. Create orphan branch 'init'
2. git add -A (all currently tracked + untracked non-ignored files)
3. Commit with the canonical initial message
4. Replace 'main' with 'init'
5. Delete old v1.0.0 tag, recreate on new commit
6. Force-push main + tag to Gitea and GitHub
"""
import os, sys, subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# --- Load .env ---
env_path = REPO.parent.parent / '.env'
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

GITEA_PUSH = f"{GITEA_URL.replace('://', f'://{GITEA_USER}:{GITEA_TOK}@')}/{GITEA_USER}/{REPO_NAME}.git"
GH_PUSH    = f'https://{GH_USER}:{GH_TOK}@github.com/{GH_USER}/{REPO_NAME}.git'

COMMIT_MSG = 'chore: initial commit (extracted from Launchers monorepo)'
TAG        = 'v1.0.0'

def git(*args, check=True):
    r = subprocess.run(['git', '-C', str(REPO)] + list(args),
                       capture_output=True, text=True)
    if check and r.returncode != 0:
        print(f'[git {" ".join(args)}] stderr: {r.stderr.strip()}', file=sys.stderr)
        sys.exit(1)
    return r

# 1. Crea branch orfano
print('[1] Creo branch orfano "init"...')
git('checkout', '--orphan', 'init')

# 2. Stage tutto (include file attualmente tracciati + non-ignorati)
print('[2] Stage di tutti i file...')
git('add', '-A')

# Verifica cosa verrà committato
r = git('status', '--short')
print(r.stdout.strip())

# 3. Commit
print('[3] Commit unico...')
# Configura autore se non già globale
r = git('commit', '-m', COMMIT_MSG)
print(f'[ok] {r.stdout.strip()}')

# 4. Sostituisci main con init
print('[4] Sostituisco branch main...')
git('branch', '-D', 'main')
git('branch', '-m', 'init', 'main')

# 5. Elimina vecchio tag e ricrealo sul nuovo commit
print(f'[5] Ricreo tag {TAG}...')
git('tag', '-d', TAG, check=False)  # potrebbe non esistere
git('tag', TAG)

# Verifica finale
r = git('log', '--oneline')
print(f'[log] {r.stdout.strip()}')
r = git('tag', '--list')
print(f'[tags] {r.stdout.strip()}')

# 6. Force push a Gitea
print('[6] Force push a Gitea...')
r = subprocess.run(
    ['git', '-C', str(REPO), 'push', '--force', GITEA_PUSH, 'main', TAG],
    capture_output=True, text=True
)
if r.returncode != 0:
    print(f'[FAIL] Gitea: {r.stderr}', file=sys.stderr); sys.exit(1)
print(f'[ok] Gitea: {r.stderr.strip()}')

# 7. Force push a GitHub
print('[7] Force push a GitHub...')
r = subprocess.run(
    ['git', '-C', str(REPO), 'push', '--force', GH_PUSH, 'main', TAG],
    capture_output=True, text=True
)
if r.returncode != 0:
    print(f'[FAIL] GitHub: {r.stderr}', file=sys.stderr); sys.exit(1)
print(f'[ok] GitHub: {r.stderr.strip()}')

# 8. Aggiorna origin/main
print('[8] Aggiorno remote tracking...')
r = subprocess.run(
    ['git', '-C', str(REPO), 'remote', 'set-head', 'origin', 'main'],
    capture_output=True, text=True
)
# Fetch per sincronizzare origin/main con il nuovo HEAD
gitea_fetch = GITEA_PUSH
r2 = subprocess.run(
    ['git', '-C', str(REPO), 'fetch', '--force', gitea_fetch, 'main:refs/remotes/origin/main'],
    capture_output=True, text=True
)

# Verifica stato finale
r = git('log', '--oneline', '-3')
print(f'\n[FINAL LOG]\n{r.stdout.strip()}')
r = git('status', '-sb')
print(f'[FINAL STATUS]\n{r.stdout.strip()}')

print('\n[DONE] Repository ridotto a singolo commit iniziale.')
