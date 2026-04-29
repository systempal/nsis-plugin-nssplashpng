"""
Push latest commit to Gitea + GitHub directly,
then disable Gitea Actions on the repo (keep GitHub Actions enabled).
"""
import urllib.request, urllib.error, json, os, sys, subprocess

# --- Load .env from workspace root ---
env_path = os.path.join(os.path.dirname(__file__), '..', '..', '..', '.env')
env_path = os.path.normpath(env_path)
if os.path.exists(env_path):
    with open(env_path, encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#') and '=' in line:
                k, v = line.split('=', 1)
                os.environ.setdefault(k.strip(), v.strip())

GITEA_URL   = os.environ['GITEA_URL']
GITEA_USER  = os.environ['GITEA_USER']
GITEA_TOK   = os.environ['GITEA_TOKEN']
GH_USER     = os.environ['GITHUB_USER']
GH_TOK      = os.environ['GITHUB_TOKEN']
REPO        = 'nsis-plugin-nssplashpng'

STAGING = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))

def api_call(url, method='GET', data=None, headers=None):
    body = json.dumps(data).encode() if data else None
    req  = urllib.request.Request(url, data=body, headers=headers or {}, method=method)
    try:
        with urllib.request.urlopen(req) as r:
            raw = r.read()
            return r.status, json.loads(raw) if raw else {}
    except urllib.error.HTTPError as e:
        raw = e.read()
        return e.code, json.loads(raw) if raw else {}

# --- 1. Push to Gitea ---
print('[1] Push to Gitea...')
gitea_push_url = f"{GITEA_URL.replace('://', f'://{GITEA_USER}:{GITEA_TOK}@')}/{GITEA_USER}/{REPO}.git"
r = subprocess.run(['git', '-C', STAGING, 'push', gitea_push_url, 'main'],
                   capture_output=True, text=True)
if r.returncode != 0:
    print(f'[FAIL] Gitea push: {r.stderr}', file=sys.stderr); sys.exit(1)
print(f'[ok] Gitea push: {r.stderr.strip() or "up to date"}')

# --- 2. Push to GitHub ---
print('[2] Push to GitHub...')
gh_push_url = f'https://{GH_USER}:{GH_TOK}@github.com/{GH_USER}/{REPO}.git'
r = subprocess.run(['git', '-C', STAGING, 'push', gh_push_url, 'main'],
                   capture_output=True, text=True)
if r.returncode != 0:
    print(f'[FAIL] GitHub push: {r.stderr}', file=sys.stderr); sys.exit(1)
print(f'[ok] GitHub push: {r.stderr.strip() or "up to date"}')

# --- 3. Disable Gitea Actions ---
print('[3] Disabling Gitea Actions...')
gitea_hdrs = {'Authorization': f'token {GITEA_TOK}', 'Content-Type': 'application/json'}
s, resp = api_call(
    f'{GITEA_URL}/api/v1/repos/{GITEA_USER}/{REPO}',
    method='PATCH',
    data={'has_actions': False},
    headers=gitea_hdrs
)
if s in (200, 201, 204):
    actions_status = resp.get('has_actions', '?')
    print(f'[ok] Gitea Actions disabled (has_actions={actions_status})')
else:
    print(f'[warn] PATCH repo returned {s}: {resp}')
    print('[warn] Trying via repo settings API...')
    # Alternative: some Gitea versions use a different field name
    s2, resp2 = api_call(
        f'{GITEA_URL}/api/v1/repos/{GITEA_USER}/{REPO}',
        method='PATCH',
        data={'actions': False},
        headers=gitea_hdrs
    )
    print(f'[info] actions=False attempt: {s2}: {resp2.get("has_actions", resp2.get("actions", "?"))}')

print('[DONE]')
