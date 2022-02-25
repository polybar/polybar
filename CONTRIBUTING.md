# Contributing

First of all, thank you very much for considering contributing to polybar. You
are awesome! :tada:

**Table of Contents:**
* [Bug Reports](#bug-reports)
* [Pull Requests](#pull-requests)
  + [Testing](#testing)
  + [Changelog](#changelog)
  + [Documentation](#documentation)
  + [Style](#style)
* [Donations](#donations)

## Bug Reports

Bugs should be reported at the polybar issue tracker, using the [bug report
template](https://github.com/polybar/polybar/issues/new?template=bug_report.yml).
Make sure you fill out all the required sections.

Before opening a bug report, please search our [issue
tracker](https://github.com/polybar/polybar/issues?q=is%3Aissue) and [known
issues page](https://github.com/polybar/polybar/wiki/Known-Issues) for your
problem to avoid duplicates.

If your issue has already been reported but is already marked as fixed and the
version of polybar you are using includes this supposed fix, feel free to open a
new issue.

You should also go through our [debugging
guide](https://github.com/polybar/polybar/wiki/Debugging-your-Config) to confirm
what you are experiencing is indeed a polybar bug and not an issue with your
configuration.
This will also help you narrow down the issue which, in turn, will help us
resolve it, if it turns out to be a bug in polybar.

If this bug was not present in a previous version of polybar and you know how
to, doing a `git bisect` and providing us with the commit ID that introduced the
issue would be immensely helpful.

## Pull Requests

If you want to start contributing to polybar, a good place to start are issues
labeled with
[help wanted](https://github.com/polybar/polybar/labels/help%20wanted)
or
[good first issue](https://github.com/polybar/polybar/labels/good%20first%20issue).

Except for small changes, PRs should always address an already open and accepted
issue.
Otherwise you run the risk of spending time implementing something and then the
PR being rejected because the feature you implemented was not actually something
we want in polybar.

Issues with any of the following labels are generally safe to start working on,
unless someone else has already claimed them:

* [bug](https://github.com/polybar/polybar/labels/bug)
* [confirmed](https://github.com/polybar/polybar/labels/confirmed)
* [good first issue](https://github.com/polybar/polybar/labels/good%20first%20issue)
* [help wanted](https://github.com/polybar/polybar/labels/help%20wanted)
* [hacktoberfest](https://github.com/polybar/polybar/labels/Hacktoberfest) (can also be worked on outside of October :wink:)

For anything else, it's a good idea to first comment under the issue to ask
whether it is something that can/should be worked on right now.
This is especially true for issues labeled with `feature` (and none of the
labels listed above), here a feature may depend on some other things being
implemented first or it may need to be split into many smaller features, because
it is too big otherwise.
In particular, this means that you should not open a feature request and
immediately start working on that feature, unless you are very sure it will be
accepted or accept the risk of it being rejected.

Things like documentation changes or refactorings, don't necessarily need an
issue associated with them.
These changes are less likely to be rejected since they don't change the
behavior of polybar.
Nevertheless, for bigger changes or when in doubt, open an issue and ask whether
such changes would be desirable.

To claim an issue, comment under it to let others know that you are working on
it.

Feel free to ask for feedback about your changes at any time.
Especially when implementing features, this can be very useful because it allows
us to make sure you are going in the direction we had envisioned for that
feature and you don't lose time on something that ultimately has to be
rewritten.
In that case, a [draft PR](https://github.blog/2019-02-14-introducing-draft-pull-requests/)
is a useful tool.

When creating a PR, please fill out the PR template.

### Testing

Your PR must pass all existing tests.
If possible, you should also add tests for the things you write.
However, this is not always possible, for example when working on modules.
But at least isolated components should be tested.

See the [testing page](https://github.com/polybar/polybar/wiki/Testing) on the
wiki for more information.
Also don't hesitate to ask for help, testing isn't that mature in polybar yet
and some things may be harder/impossible to test right now.

### Changelog

We use the [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) format for
keeping track of changes in a release.

If your PR introduces notable changes to polybar, please add them the correct
subsection in the `Unreleased` section in the `CHANGELOG.md` file at the root
of this repository.
Notable changes are any user-visible changes, like bug fixes, new config
options, changes to the build, etc., but not, for example, code cleanup that
doesn't change polybar's behavior or minor documentation changes.
One thing that also should not be added to the changelog are bugfixes for
unreleased features.

An entry in the changelog should include a link to the issue(s) that the PR
addresses, to the PR itself, and to your username:

```
- A short description of the change
  ([`#XYZ`](https://github.com/polybar/polybar/issues/XYZ),
  [`#UVW`](https://github.com/polybar/polybar/pull/UVW))
  by [@yourname](https://github.com/yourname).
```

You will first need to open the PR before you can link to it though :wink:.


If you are unsure whether something is a notable change, just add it to the
changelog and we can determine whether it is a notable change when reviewing.

### Documentation

Right now, documentation for polybar lives in two places: The GitHub wiki and
the git repo itself.

Ultimately, most of the documentation is supposed to live in the repo itself.

For now, if your PR requires documentation changes in the repo, those changes
need to be in the PR as well.

Changes on the wiki should not be made right away because the wiki should
reflect the currently released version and not the development version.
In that case, outline the documentation changes that need to be made (for
example, which new config options are available).
If your PR would introduce a lot of new documentation on the wiki, let us know
and we can decide if we want to put some of the documentation directly into the
repo.

### Style

Please read our [style guide](https://github.com/polybar/polybar/wiki/Style-Guide).

## Donations

Donations support the sustained development of polybar.
We accept donations through [our open collective page](
https://opencollective.com/polybar).
This can be either a one time or a recurring donation.

Our [blog post](https://polybar.github.io/2020/12/21/Polybar-Funding.html) lays
out the why and how behind our decision to accept donations.
