"""Sphinx documentation for AmurCore; source code checked out separately."""
from pathlib import Path

DOCS = Path(__file__).resolve().parent
project = "AmurCore"
author = "AmurCore contributors"
extensions = ["myst_parser", "sphinxcontrib.mermaid", "breathe"]
source_suffix = {".md": "markdown", ".rst": "restructuredtext"}
root_doc = "index"
myst_fence_as_directive = ["mermaid"]
myst_heading_anchors = 3
mermaid_config = {"securityLevel": "strict"}
mermaid_fullscreen = False
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]
breathe_projects = {"AmurCore": str(DOCS / "_build" / "xml")}
breathe_default_project = "AmurCore"
revision_file = DOCS / "_source_sha.txt"
source_sha = revision_file.read_text(encoding="utf-8").strip() if revision_file.exists() else "unbuilt"
html_theme = "furo"
html_title = f"AmurCore documentation · source {source_sha[:12]}"
html_baseurl = "https://ardev1161.github.io/AmurCore/"
html_static_path = ["_static"]
html_css_files = ["layout.css"]
html_theme_options = {
    "source_repository": "https://github.com/ARDev1161/AmurCore/",
    "source_branch": "documentation",
    "source_directory": "docs/",
}
