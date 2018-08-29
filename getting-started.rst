.. _getting-started:

=================
 Getting Started
=================

For getting started grab the latest dim (drm-intel-maintainer) script from::

    https://gitlab.freedesktop.org/drm/maintainer-tools/raw/master/dim

There's also a sample config file for ~/.dimrc::

    https://gitlab.freedesktop.org/drm/maintainer-tools/raw/master/dimrc.sample

Plus, there's bash completion in the same directory if you feel like using that.
Run::

    $ dim help

for tons of details about how this thing works. Also see the git repository
specific pages for details on the patch merging process for each tree. Adjust
your .dimrc to match your setup and then run::

    $ dim setup

This will also check out the latest maintainer-tools branches, so please replace
the dim you just downloaded with a symlink after this step. And by the way, if
you have improvements for dim, see `contributing
<dim.html#contributing-bug-reports-and-discussion>`_.

If you have a freedesktop.org account and plan to push things on one of the
drm-xxx repos, you should use the ssh://git.freedesktop.org/git/drm-xxx urls
when adding a remote. Note that dim will ask you to add missing remotes
automatically, and by default uses the ssh:// url format. To make these urls
without login names work, you should add a new entry in ~/.ssh/config, if that's
not yet configured::

    $ printf '\nHost git.freedesktop.org\n\tUser <username>' >> ~/.ssh/config

You should now have a main repository for patch application. The directory
corresponding to this repository is defined by DIM_REPO in your .dimrc.
You should also have directories called maintainer-tools, drm-tip (for
rebuilding the tree), and drm-rerere for some dim-internal book-keeping.

If someone else has pushed patches first resync using::

   $ dim update-branches

Since dim supports lots of different branches in different repositories you
first need to check out the right branch using::

   $ dim checkout <branch>

Applying patches is done in the main repository with::

    $ cat patch.mbox | dim apply-branch <branch>

This works like a glorified version of git apply-mbox and does basic patch
checking and adds stuff like patchwork links of the merged patch. It is
preferred to use the patch email file instead of the original patch file since
it contains some interesting headers like the message ID. When you're happy
(remember that with a shared tree any mistake is permanent and there's no
rebasing) push out the new tree with::

    $ dim push-branch <branch>

This will also rebuild a new drm-tip integration tree. For historical reasons
there's shortcut for the drm-intel specific branches for most of these commands.

Please note that if there is no specific command available from dim then you
can always use your every day tooling to get things done.
For example, if a wrong patch was applied or you need to update commit message
or fix anything else in the git repository, then just use git to do so
as usual.

Please subscribe to the https://lists.freedesktop.org/mailman/listinfo/dim-tools
mailing list if you use dim.
