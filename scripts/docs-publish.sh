#!/usr/bin/env bash
#
# docs-publish — build the documentation site and publish it to the `gh-pages` branch.
#
# WHY THIS EXISTS (2026-10-16, card t_79c842b6): .github/workflows/docs.yml did two jobs on
# GitHub's runners — VALIDATE the documentation (every example, operator coverage, the
# generated reference and the generated indexes) and DEPLOY the site with
# actions/upload-pages-artifact + actions/deploy-pages. Validation is the gate's `docs` stage
# now, and it is in gate.toml's [tier.fast], so it runs on THIS machine at every commit.
# Deployment is this script, run by a human who has the engine built. Nothing about the site
# changes: same URL, same pages, same badge. What changed is where the work happens, and that
# the site is served from a branch instead of from a workflow artifact.
#
# HOW IT COMMITS: a subtree commit, through a TEMPORARY WORKTREE.
# The gh-pages branch holds the site and nothing else, so `git log gh-pages` reads as a
# publishing history rather than a second copy of this repo's history; the first publish
# creates it as an ORPHAN branch (no parent, no main history). The commit is made in a worktree
# under $TMPDIR that is discarded on the way out, so this script never checks out, stashes or
# otherwise touches main's index, HEAD or working tree. The failure it therefore cannot have is
# losing uncommitted work to a publish.
#
# WHY THE PUSH BYPASSES THE PRE-PUSH HOOK (--no-verify): the hook runs this repo's FULL gate
# whatever refs are being pushed. gh-pages is generated content committed in a temporary
# worktree that contains no scripts/ at all: run from inside it the hook cannot even find the
# gate and fails the push, and run from main's root it would run the whole C++ suite (asan,
# tsan, package, pristine) because a badge JSON changed. The protection that matters is kept,
# by this script: a dirty tree and a HEAD that is not main are refused, and what is published is
# the site built from that tree at that moment. The gate on the CODE is still the commit and the
# push to main, exactly as before.
#
# USAGE
#   scripts/docs-publish.sh
#
# It asks for no confirmation. Unlike the kit's release.sh publish there is no public release
# being cut here, no tag, and nothing irreversible: the previous gh-pages commit stays in the
# branch's history, so a bad publish is undone by re-running this script on the right commit.
set -uo pipefail
# gate-env.sh is sourced for the same reason every stage script sources it: CI_BUILD_DIR (and
# .ci.env) decide WHICH binary is graded, and the site must be built from the binary the gate's
# `docs` stage grades rather than from a second opinion about where the build is. It also unsets
# GIT_INDEX_FILE on the way in. This is NOT a gate stage — nothing in gate.toml runs it and it
# prints no gate verdict.
. "$(dirname "$0")/gate-env.sh"

REPO_SLUG="HarryPehkonen/Computo"        # the Pages API and the site URL both name this repo
SITE_URL="https://harrypehkonen.github.io/Computo/"

die()   { printf 'docs-publish: %s\n' "$*" >&2; exit 1; }
info()  { printf '  %s\n' "$*"; }
head_() { printf '\n== %s ==\n' "$*"; }
have()  { command -v "$1" >/dev/null 2>&1; }

tmp_root=$(mktemp -d "${TMPDIR:-/tmp}/computo-docs-publish-XXXXXX") || die "mktemp failed"
wt="$tmp_root/gh-pages"                  # the worktree the gh-pages commit is made in
site="$tmp_root/site"                    # the content that goes into it

# The worktree is a real checkout of gh-pages and is removed by git; the temp root around it is
# ours. On any exit — success, failure, Ctrl-C — neither is left behind, and no half-made
# worktree survives to confuse the NEXT publish.
cleanup() {
    if [ -d "$wt" ] && ! git worktree remove --force "$wt" 2>/dev/null; then
        printf 'docs-publish: could not remove the worktree at %s (git worktree prune will)\n' "$wt" >&2
    fi
    rm -rf "$tmp_root"
}
trap cleanup EXIT

# ------------------------------------------------------------------ 1. guards
head_ "guards"
git rev-parse --git-dir >/dev/null 2>&1 || die "not a git repository"
[ -z "$(git status --porcelain)" ] \
    || die "working tree is dirty: commit first — the site is built from the tree, and a published page that matches no commit is a page nobody can reproduce"
[ "$(git rev-parse --abbrev-ref HEAD)" = "main" ] \
    || die "not on main (on '$(git rev-parse --abbrev-ref HEAD)'): gh-pages publishes what main says"
git fetch --quiet origin main || die "cannot reach origin (git fetch origin main failed)"
git rev-parse --verify -q origin/main >/dev/null || die "no origin/main to compare against"
if [ "$(git rev-parse HEAD)" != "$(git rev-parse origin/main)" ]; then
    # BEHIND is a refusal: publishing an older tree would state, on a public page, something main
    # has already moved past. AHEAD is not: that content is HEAD's, which is what the next push
    # to main makes official, and being able to look at the site before that push is a reason for
    # a local publish action to exist.
    git merge-base --is-ancestor HEAD origin/main \
        && die "origin/main is ahead of HEAD: pull first — publishing now would publish an older tree"
    info "HEAD is ahead of origin/main by $(git rev-list --count origin/main..HEAD) commit(s): publishing HEAD"
fi
have cmake   || die "cmake is not installed"
have python3 || die "python3 is not installed (the documentation pipeline needs it)"
python3 -c 'import yaml' >/dev/null 2>&1 \
    || die "python3 has no PyYAML (the index and badge generation need it; Debian/Ubuntu: apt install python3-yaml)"
have gh      || die "gh is not installed (the Pages API call and the read-back need it)"
have curl    || die "curl is not installed (the final check fetches the published page)"
binary="$CI_BUILD_DIR/computo"
[ -x "$binary" ] \
    || die "no executable at $binary — run scripts/build.sh first (scripts/gate.sh --tier fast builds it as it runs the docs stage)"
info "will publish the site for $(git rev-parse --short HEAD) on main, built from $binary"

# ------------------------------------------------------------------ 2. generate
head_ "generate (the two steps the workflow's deploy job ran, in its order)"
cmake --build "$CI_BUILD_DIR" --target docs-generate > "$CI_LOG_DIR/docs-publish-generate.log" 2>&1 \
    || { tail -20 "$CI_LOG_DIR/docs-publish-generate.log" | sed 's/^/      /'
         die "cmake --build $CI_BUILD_DIR --target docs-generate failed (log: $CI_LOG_DIR/docs-publish-generate.log)"; }
python3 docs/generate-indexes.py > "$CI_LOG_DIR/docs-publish-indexes.log" 2>&1 \
    || { tail -20 "$CI_LOG_DIR/docs-publish-indexes.log" | sed 's/^/      /'
         die "python3 docs/generate-indexes.py failed (log: $CI_LOG_DIR/docs-publish-indexes.log)"; }
info "docs/LANGUAGE_REFERENCE.md, docs/alpha/index.md and docs/task/index.md regenerated"
# Both steps write TRACKED files. The guard above proved the tree was clean, so anything dirty
# now is content no commit has, and publishing it would put a reference on the site that the
# repository does not state. The `docs` stage fails on exactly this; this is the publish
# action's own copy of the rule, because a publish can run long after the last commit.
if [ -n "$(git status --porcelain)" ]; then
    git status --porcelain | sed 's/^/      /'
    die "regeneration changed tracked files (above) — commit them, then run this again"
fi

# ------------------------------------------------------------------ 3. the site content
head_ "site content"
mkdir -p "$site/alpha" "$site/task"
# index.html: the heredoc the workflow carried, moved here byte for byte — the YAML block
# indent is gone and nothing else about it is touched.
cat > "$site/index.html" <<'SITE_INDEX_EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Computo Documentation</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <link href="https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/themes/prism-tomorrow.min.css" rel="stylesheet">
    <style>
        .hero { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 4rem 0; }
        .code-block { background: #2d3748; border-radius: 8px; }
        .nav-pills .nav-link.active { background-color: #667eea; }
    </style>
</head>
<body>
    <div class="hero">
        <div class="container">
            <div class="row">
                <div class="col-lg-8">
                    <h1 class="display-4 fw-bold">Computo</h1>
                    <p class="lead">A safe, sandboxed, thread-safe JSON-native data transformation engine</p>
                    <div class="d-flex gap-3">
                        <a href="#language-reference" class="btn btn-light btn-lg">Language Reference</a>
                        <a href="https://github.com/HarryPehkonen/Computo" class="btn btn-outline-light btn-lg">GitHub</a>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <div class="container mt-5">
        <div class="row">
            <div class="col-md-3">
                <div class="nav flex-column nav-pills" role="tablist">
                    <button class="nav-link active" data-bs-toggle="pill" data-bs-target="#overview">Overview</button>
                    <button class="nav-link" data-bs-toggle="pill" data-bs-target="#language-reference">Language Reference</button>
                    <button class="nav-link" data-bs-toggle="pill" data-bs-target="#alphabetical-index">Alphabetical Index</button>
                    <button class="nav-link" data-bs-toggle="pill" data-bs-target="#task-index">Task-Based Index</button>
                    <button class="nav-link" data-bs-toggle="pill" data-bs-target="#verification-process">Verification Process</button>
                    <button class="nav-link" data-bs-toggle="pill" data-bs-target="#quick-start">Quick Start</button>
                </div>
            </div>
            <div class="col-md-9">
                <div class="tab-content">
                    <div class="tab-pane fade show active" id="overview">
                        <h2>What is Computo?</h2>
                        <p>Computo is a functional programming language for JSON data transformations using a Lisp-like syntax expressed in JSON.</p>

                        <div class="row mt-4">
                            <div class="col-md-6">
                                <h4>Key Features</h4>
                                <ul>
                                    <li>JSON-native syntax</li>
                                    <li>Thread-safe execution</li>
                                    <li>Tail call optimization</li>
                                    <li>46 built-in operators</li>
                                    <li>Multi-input support</li>
                                    <li>Interactive REPL with debugging</li>
                                </ul>
                            </div>
                            <div class="col-md-6">
                                <h4>Example</h4>
                                <pre class="code-block p-3"><code class="language-json">["+", 1, 2, 3]</code></pre>
                                <p><small>Result: <code>6.0</code></small></p>
                            </div>
                        </div>
                    </div>

                    <div class="tab-pane fade" id="language-reference">
                        <div id="language-reference-content">Loading...</div>
                    </div>

                    <div class="tab-pane fade" id="alphabetical-index">
                        <div id="alphabetical-index-content">Loading...</div>
                    </div>

                    <div class="tab-pane fade" id="task-index">
                        <div id="task-index-content">Loading...</div>
                    </div>

                    <div class="tab-pane fade" id="verification-process">
                        <div id="verification-process-content">Loading...</div>
                    </div>

                    <div class="tab-pane fade" id="quick-start">
                        <h2>Quick Start</h2>
                        <h3>Installation</h3>
                        <pre class="code-block p-3"><code class="language-bash">git clone https://github.com/HarryPehkonen/Computo.git
cd Computo
cmake -B build
cmake --build build</code></pre>

                        <h3>Basic Usage</h3>
                        <pre class="code-block p-3"><code class="language-bash"># Execute a script
./build/computo --script script.json input.json

# Start interactive REPL
./build/computo --repl</code></pre>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/components/prism-core.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/plugins/autoloader/prism-autoloader.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/marked/marked.min.js"></script>
    <script>
        // Load and render markdown content
        async function loadMarkdown(file, targetId) {
            try {
                const response = await fetch(file);
                const markdown = await response.text();
                const html = marked.parse(markdown);
                document.getElementById(targetId).innerHTML = html;
                Prism.highlightAllUnder(document.getElementById(targetId));
            } catch (error) {
                document.getElementById(targetId).innerHTML = '<p class="text-danger">Failed to load content.</p>';
            }
        }

        // Load content when tabs are shown
        document.addEventListener('DOMContentLoaded', function() {
            const tabTriggers = document.querySelectorAll('[data-bs-toggle="pill"]');
            tabTriggers.forEach(trigger => {
                trigger.addEventListener('shown.bs.tab', function(event) {
                    const target = event.target.getAttribute('data-bs-target');
                    if (target === '#language-reference' && !document.getElementById('language-reference-content').innerHTML.includes('<h1>')) {
                        loadMarkdown('LANGUAGE_REFERENCE.md', 'language-reference-content');
                    } else if (target === '#alphabetical-index' && !document.getElementById('alphabetical-index-content').innerHTML.includes('<h1>')) {
                        loadMarkdown('alpha/index.md', 'alphabetical-index-content');
                    } else if (target === '#task-index' && !document.getElementById('task-index-content').innerHTML.includes('<h1>')) {
                        loadMarkdown('task/index.md', 'task-index-content');
                    } else if (target === '#verification-process' && !document.getElementById('verification-process-content').innerHTML.includes('<h1>')) {
                        loadMarkdown('SYNTAX_VERIFICATION_PROCESS.md', 'verification-process-content');
                    }
                });
            });
        });
    </script>
</body>
</html>
SITE_INDEX_EOF
cp docs/LANGUAGE_REFERENCE.md docs/SYNTAX_VERIFICATION_PROCESS.md README.md "$site/" \
    || die "could not copy the documentation into $site"
cp docs/alpha/index.md "$site/alpha/" || die "docs/alpha/index.md is missing"
cp docs/task/index.md  "$site/task/"  || die "docs/task/index.md is missing"
# .nojekyll, and it is not cosmetic: a branch-served Pages site goes through Jekyll unless the
# published root says otherwise, and Jekyll CONVERTS markdown rather than serving it. index.html
# fetches LANGUAGE_REFERENCE.md, alpha/index.md and task/index.md as plain files, so without
# this marker those three tabs would 404. The workflow's artifact deploy never ran Jekyll, which
# is why this file had no reason to exist until publishing moved to a branch (the final check
# below re-measures it on the live site).
: > "$site/.nojekyll"
# The coverage badge. The workflow hardcoded "/46 operators"; both numbers are free here and the
# gate's `docs` stage has already proved they agree, so they are counted instead of remembered.
totals=$(python3 - "$binary" docs/operators.yaml <<'COUNTS_EOF'
import json, subprocess, sys, yaml
implemented = json.loads(subprocess.run([sys.argv[1], "--list-operators"],
                                        capture_output=True, text=True, check=True).stdout)
documented = yaml.safe_load(open(sys.argv[2], encoding="utf-8"))["operators"]
print(len(implemented), len(documented))
COUNTS_EOF
) || die "could not count the operators (from --list-operators and docs/operators.yaml)"
read -r implemented documented <<< "$totals"
color=brightgreen
[ "$implemented" = "$documented" ] || color=red
printf '{ "schemaVersion": 1, "label": "docs coverage", "message": "%s/%s operators", "color": "%s" }\n' \
    "$implemented" "$documented" "$color" > "$site/coverage-badge.json"
printf '  %s\n' "$(find "$site" -type f | sed "s|^$site/|    |" | sort)"

# ------------------------------------------------------------------ 4. publish
head_ "publish to gh-pages"
if git rev-parse --verify -q refs/remotes/origin/gh-pages >/dev/null; then
    git fetch --quiet origin gh-pages || die "cannot fetch origin gh-pages"
    git worktree add -B gh-pages "$wt" FETCH_HEAD >/dev/null 2>&1 \
        || die "could not create a worktree on gh-pages (is gh-pages checked out somewhere?)"
    msg="docs: publish $(git rev-parse --short HEAD)"
else
    # First publish: an orphan branch, so gh-pages carries the site's history and none of the
    # engine's. `git worktree add --orphan` needs git >= 2.42.
    git worktree add --orphan -b gh-pages "$wt" >/dev/null 2>&1 \
        || die "could not create the orphan gh-pages worktree (needs git >= 2.42 — on an older git, make the branch by hand and copy $site to its root)"
    msg="docs: publish the site for $(git rev-parse --short HEAD) (first commit on the orphan gh-pages branch)"
fi
# The worktree holds a checkout of the PREVIOUS site. The new content replaces it wholesale, so a
# page that left docs/ leaves the site too, instead of living on in the branch. `.git` is left
# alone: in a linked worktree it is a FILE pointing back at the main repository, and deleting it
# would break the very worktree the commit is made in (the first publish has nothing to delete).
find "$wt" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +
cp -a "$site/." "$wt/"
git -C "$wt" add -A
if git -C "$wt" diff --cached --quiet; then
    info "the content is byte-identical to the published site: nothing to commit"
else
    git -C "$wt" commit -q -m "$msg" || die "the gh-pages commit failed (is user.email configured?)"
    info "$(git -C "$wt" log -1 --format='%h %s')"
    git -C "$wt" push --no-verify origin gh-pages \
        || die "could not push gh-pages (if the remote moved, fetch and run this again)"
    info "pushed gh-pages"
fi

# ------------------------------------------------------------------ 5. Pages source + read-back
head_ "GitHub Pages source"
gh api "repos/$REPO_SLUG/pages" -X PUT \
    -f build_type=legacy -f "source[branch]=gh-pages" -f "source[path]=/" >/dev/null \
    || die "the Pages API refused the switch (the token needs repo scope and Pages admin)"
state=$(gh api "repos/$REPO_SLUG/pages" --jq '"\(.build_type) \(.source.branch) \(.source.path)"') \
    || die "could not read the Pages configuration back"
[ "$state" = "legacy gh-pages /" ] || die "GitHub Pages reports '$state', not 'legacy gh-pages /'"
info "GitHub Pages: build_type=legacy, source=gh-pages /  (read back, not assumed)"

# ------------------------------------------------------------------ 6. the live site
head_ "the live site"
# Both are graded, because the browser fetches both: index.html is the page, and
# LANGUAGE_REFERENCE.md is fetched BY it — the file that would 404 without .nojekyll. A new
# branch build takes a few seconds to appear, hence the retries.
index_code=000
ref_code=000
for _ in $(seq 1 12); do
    index_code=$(curl -s -o /dev/null -w '%{http_code}' "$SITE_URL") || index_code=000
    ref_code=$(curl -s -o /dev/null -w '%{http_code}' "${SITE_URL}LANGUAGE_REFERENCE.md") || ref_code=000
    [ "$index_code" = "200" ] && [ "$ref_code" = "200" ] && break
    sleep 10
done
[ "$index_code" = "200" ] || die "$SITE_URL answered $index_code (index.html)"
[ "$ref_code" = "200" ] \
    || die "${SITE_URL}LANGUAGE_REFERENCE.md answered $ref_code — that file is fetched BY index.html (is .nojekyll in the published root?)"
info "$SITE_URL serves: index.html and LANGUAGE_REFERENCE.md both 200 as of $(date -u +%FT%TZ)"
