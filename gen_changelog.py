#!/usr/bin/env python3

# Generate and upload changelog

from git import Repo, exc
from github import Github
import os
import sys

upload_changelog = True

try:
    current_tag = os.environ['TRAVIS_TAG']
    print('TRAVIS_TAG is set to {}'.format(current_tag))
except KeyError:
    print('TRAVIS_TAG not set - not uploading changelog')
    current_tag = 'HEAD'
    upload_changelog = False

try:
    api_key = os.environ['GITHUB_OAUTH_TOKEN']
except KeyError:
    print('GITHUB_OAUTH_TOKEN not set - not uploading changelog')
    api_key = None
    upload_changelog = False

try:
    repo_slug = os.environ['TRAVIS_REPO_SLUG']
except KeyError:
    print('TRAVIS_REPO_SLUG not set - cannot determine remote repository')
    repo_slug = ''
    exit(1)

if len(sys.argv) > 1:
    repo_path = sys.argv[1]
else:
    repo_path = '.'

print('Opening repository at {}'.format(repo_path))
repo = Repo(repo_path)
git = repo.git()
try:
    git.fetch('--unshallow')
except exc.GitCommandError:
    print('Repository already unshallowed')
if current_tag != 'HEAD':
    print('Attempting to get base tag from current tag')
    base_tag = current_tag.split('-')[0]
    print('Base tag set to {}'.format(base_tag))
else:
    description = git.describe()
    print('Attempting to get base tag from git description: {}'.format(description))
    base_tag = description.split('-')[0]
    print('Base tag set to {}'.format(base_tag))

changelog = git.log('{}...{}'.format(base_tag, current_tag), '--pretty=format:* %H %an "%s"\n')
print('Current changelog: \n{}'.format(changelog))

# Only interact with Github if uploading is enabled
if upload_changelog:
    gh = Github(api_key)
    gh_repo = gh.get_repo(repo_slug)
    # Release ID should match our tag ID
    gh_release = gh_repo.get_release(current_tag)
    gh_body = gh_release.body
    if gh_body is None:
        gh_body = ''
    gh_release.body = '{}\nChanges between {} and {}:\n{}'.format(gh_body, base_tag, current_tag, changelog)
    print('New release body: {}'.format(gh_release.body))
    gh_release.draft = True
    gh_release.update()