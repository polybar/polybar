Release Workflow
================

We try to follow `Semantic Versioning <https://semver.org/>`_ in this project.
Patch releases (e.g. ``3.3.X``) contain only bug fixes. Minor releases (e.g.
``3.X.0``) can have backwards-compatible features. And major releases (
``X.0.0``) can introduce incompatible changes.

.. note::

  This document replaces the "`Release Guidelines
  <https://github.com/polybar/polybar/wiki/Release-Guidelines>`_" on the wiki
  that we used between 3.2.0 and 3.4.3. Starting with 3.5.0, we will follow
  the workflow described here to publish releases.

Polybar uses the `OneFlow
<https://www.endoflineblog.com/oneflow-a-git-branching-model-and-workflow>`_
branching model for publishing new releases and introducing hotfixes.

The way we accept code from contributors does not change: Contributors fork
polybar, commit their changes to a new branch and open a PR to get that branch
merged.
After reviewing and approving the changes, a maintainer "merges" the PR.
"Merging" is done in the GitHub UI by either rebasing or squashing the
changes.
Regular merging is disabled because we do not want merge a merge commit for
every PR.

This document is mainly concerned with how to properly release a new version of
polybar.
For that reason this might not be of interest to you, if you are not a
maintainer, but feel free to read on anyway.

Drafting a new Release
----------------------

There a two processes for how to draft a new release. The process for major and
minor versions is the same as they both are "regular" releases.
Patch releases are triggered by bugfixes that cannot wait until the next regular
release and have a slightly different workflow.

Regular Releases (Major, Minor)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Regular releases are created once we find that ``master`` is in a stable state
and that there are enough new features to justify a new release.
A release branch ``release/X.Y.0`` is branched off of a commit on ``master``
that contains all the features we want in the release, this branch is pushed to
the official repository.
For example for version ``3.5.0`` the branch ``release/3.5.0`` would be created:

.. code-block:: shell

  git checkout -b release/3.5.0 <commit>

The release branch should typically only exist for at most a few days.

Hotfix Releases (Patch)
~~~~~~~~~~~~~~~~~~~~~~~

A hotfix release is created whenever we receive a fix for a bug that we believe
should be released immediately instead of it only being part of the next regular
release.
Generally any bugfix qualifies, but it is up to the maintainers to decide
whether a hotfix release should be created.

The hotfix release branch ``hotfix/X.Y.Z`` is created by branching off at the
previous release tag (``X.Y.Z-1``).
For example, if the latest version is ``3.5.2``, the next hotfix will be on
branch ``hotfix/3.5.3``:

.. code-block:: shell

  git checkout -b hotfix/3.5.3 3.5.2

Since the PRs for such bugfixes are often not created by maintainers, they will
often not be based on the latest release tag, but just be branched off
``master`` because contributors don't necessarily know about this branching
model and also may well not know whether a hotfix will be created for a certain
bugfix.

.. TODO create contributor page that describes where to branch off. And link to
   that page.

In case a PR containing a bugfix that is destined for a patch release is not
branched off the previous release, a maintainer creates the proper release
branch and cherry-picks the bugfix commits.

.. note::

  Alternatively, the contributor can also ``git rebase --onto`` to base the
  branch off the previous release tag. However, in most cases it makes sense for
  a maintainer to create the release branch since they will also need to create
  a `Release PR`_ for it.

Once the release branch is created and contains the right commits, the
maintainer should follow `Publishing a new Release`_ to finish this patch
release.

If multiple bugfixes are submitted in close succession, they can all be
cherry-picked onto the same patch release branch to not create many individual
release with only a single fix.
The maintainer can also decide to leave the release branch for this patch
release open for a week in order to possibly combine multiple bugfixes into a
single release.

Publishing a new Release
------------------------

The process for publishing a release is the same for all release types. It goes
as follows:

* A `Release PR`_ is created for the release. This PR MUST NOT be merged in
  GitHub's interface, it is only here for review, merging happens at the
  commandline.
* After approval, a signed git tag without message is created locally at the
  tip of the release branch and pushed:

.. code-block:: shell

  git tag -m "" -s X.Y.Z <release-branch>
  git push --tags

* A `draft release`_ targetting the new tag is created in GitHub's release
  publishing tools and published.
* After the tag is created, the release branch is manually merged into
  ``master``.
  Here it is vitally important that the history of the release branch does not
  change and so we use ``git merge``. We do it manually because using ``git
  merge`` is disabled on PRs.

.. code-block:: shell

  git checkout master
  git merge <release-branch>
  git push origin

* After the tag is created, the release branch can be deleted with ``git push
  origin :<release-branch>``.
* Work through the `After-Release Checklist`_.

Here ``<release-branch>`` is either a ``release/X.Y.0`` branch or a
``hotfix/X.Y.Z`` branch.

Release PR
~~~~~~~~~~

The final state of the release branch is prepared as a draft PR on
GitHub.
That PR is not merged from the GitHub interface though.

The release PR must do the following things:

* Write any missing migration guides for:

  * Deprecated or removed options
  * New features that it might be worth migrating to
* Have a release commit at its tip with the message ``Version X.Y.Z`` and the following changes

  * Finalizes the `Changelog`_ for this release
  * Updates the version number in ``version.txt``

Changelog
~~~~~~~~~

The ``CHANGELOG.md`` file at the root of the repo should already contain all the
changes for the upcoming release in a format based on
`keep a changelog <https://keepachangelog.com/en/1.0.0/>`_.
For each release those changes should be checked to make sure we did not miss
anything.

For all releases, a new section of the following form should be created below
the ``Unreleased`` section:

.. code-block:: md

  ## [X.Y.Z] - YYYY-MM-DD

In addition, the reference link for the release should be added and the
reference link for the unreleased section should be updated at the bottom of the
document:

.. code-block:: md

  [Unreleased]: https://github.com/polybar/polybar/compare/X.Y.Z...HEAD
  [X.Y.Z]: https://github.com/polybar/polybar/releases/tag/X.Y.Z

Since the release tag doesn't exist yet, both of these links will be invalid
until the release is published.

All changes from the ``Unreleased`` section that apply to this release should be
moved into the new release section.
For regular releases this is generally the entire ``Unreleased`` section, while
for patch releases it will only be a few entries.

The contents of the release section can be copied into the draft release in
GitHub's release tool with a heading named ``## Changelog``.

Since major releases generally break backwards compatibility in some way, their
changelog should also prominently feature precisely what breaking changes were
introduced. If suitable, maybe even separate documentation dedicated to the
migration should be written.

Draft Release
~~~~~~~~~~~~~

On `GitHub <https://github.com/polybar/polybar/releases/new>`_ a new release
should be drafted.
The release targets the git tag that was just pushed, the name of the release
and the tag is simply the release number.

The content of the release message should contain the changelog copied from
``CHANGELOG.md`` under the heading ``## Changelog``.
In addition using GitHub's "Auto-generate release notes" feature, the list of
new contributors should be generated and put at the end of the release notes.
The generated list of PRs can be removed.

For minor and major releases, add a link to the migration guide directly under
the ``## Changelog`` header:

.. code-block:: markdown

  **[Migration Guide](https://polybar.readthedocs.io/en/stable/migration/X.Y/index.html)**

At the bottom, check the two boxes "Set as the latest release" and "Create a
discussion for this release" (select the category "Announcements").

After-Release Checklist
~~~~~~~~~~~~~~~~~~~~~~~

* Verify the release archive (see `Verify Release`_)
* Update the Wiki

  * Make sure all the new functionality is documented
  * Mark deprecated features appropriately (see `Deprecations`_)
  * Remove all "unreleased" notes (not for patch releases)
* Inform packagers of new release in :issue:`1971`. Mention any dependency
  changes and any changes to the build workflow. Also mention any new files are
  created by the installation.
* Create a PR that updates the AUR ``PKGBUILD`` file for the ``polybar-git``
  package (push after the release archive is uploaded).
* Close the `GitHub Milestone <https://github.com/polybar/polybar/milestones>`_
  for the new release and move open issues (if any) to a later release.
* Activate the version on `Read the Docs
  <https://readthedocs.org/projects/polybar/versions/>`_ and deactivate all
  previous versions for the same minor release (e.g. for 3.5.4, deactivate all
  other 3.5.X versions).

Verify Release
~~~~~~~~~~~~~~

Confirm that the release archive was added to the release.
We have a GitHub action workflow called 'Release Workflow' that on every
release automatically creates a release archive, uploads it to the release,
and adds a 'Download' section to the release body.
If this fails for some reason, it should be triggered manually.

Afterwards, download the archive, verify its hash, and sign it:

.. code-block:: shell

  gpg --armor --detach-sign polybar-X.Y.Z.tar.gz

Finally, upload the generated ``polybar-X.Y.Z.tar.gz.asc`` to the GitHub
release.

Deprecations
~~~~~~~~~~~~

If any publicly facing part of polybar is being deprecated, it should be marked
as such in the code, through warnings/errors in the log, and by comments in the
wiki. Every deprecated functionality is kept until the next major release and
removed there, unless it has not been deprecated in a minor release before.
