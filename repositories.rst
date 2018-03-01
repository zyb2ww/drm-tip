.. _repositories:

===========================
 Repositories and Branches
===========================

All the relevant repositories and branches are described below. For the current
list of maintainers, mailing lists, etc. please refer to MAINTAINERS_.

.. _MAINTAINERS: https://cgit.freedesktop.org/drm/drm-tip/plain/MAINTAINERS

The Upstream Linux Kernel Repository
------------------------------------

See upstream_ for repository details. Maintained by Linus Torvalds.

.. _upstream: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/

master
~~~~~~

Linus' master, the upstream, or mainline. This is where all features from all
subsystems, including DRM and i915, are merged.

The upstream follows a single branch, time-based development model, with a new
kernel release occurring roughly every 10 weeks. New features are merged from
subsystem trees during the two week merge window immediately following a kernel
release. After the merge window, the new development kernel is stabilized by
only merging fixes until the next kernel release. During development, there's a
new release candidate (-rc) kernel each week.

The Upstream DRM Subsystem Repository
-------------------------------------

See `drm upstream`_ for repository details. Maintained by Dave Airlie of Red
Hat. Consists mostly of ``drivers/gpu/drm`` and ``include/drm``.

.. _drm upstream: https://cgit.freedesktop.org/~airlied/linux/

drm-next
~~~~~~~~

This is the branch where all new features for the DRM core and all the GPU
drivers, including drm/i915, are merged.

The drm-next branch is closed for new features at around -rc5 timeframe of the
current development kernel in preparation for the upcoming merge window for the
next kernel, when drm-next gets merged to Linus' master. Thus there's a
stabilization period of about 3-5 weeks during which only bug fixes are merged
to drm-next.

drm-fixes
~~~~~~~~~

This is the branch where all the fixes for the DRM core and all the GPU drivers
for the current development kernels are merged. drm-fixes is usually merged to
Linus' master on a weekly basis.

.. _drm-misc-repository:

The DRM Misc Repository
-----------------------

See drm-misc_ for repository details. Maintained by Daniel Vetter, Gustavo
Padovan, and Sean Paul, with a large pool of committers.

.. _drm-misc: https://cgit.freedesktop.org/drm/drm-misc

drm-misc-next
~~~~~~~~~~~~~

This is the main feature branch where most of the patches land. This branch is
always open to "hide" the merge window from developers. To avoid upsetting
linux-next and causing mayhem in the merge window, in general no pull requests
are sent to upstream after rc6 of the current kernel release. Outside of that
feature freeze period, pull requests are sent to upstream roughly every 1-2
weeks, to avoid too much coordination pains.

If you're unsure, apply your patch here, it can always be cherry-picked to one
of the -fixes branches later on. But in contrast to the drm-intel flow
cherry-picking is not the default.

drm-misc-next-fixes
~~~~~~~~~~~~~~~~~~~

During the time between rc6 of kernel version X and rc1 of X+1, drm-misc-next
will be targeting kernel version X+2 and drm-misc-fixes still targets kernel
version X.  This branch is for fixes to bugs introduced in the drm-misc-next
pull request that was sent for X+1, which aren't present in the drm-misc-fixes
branch.

drm-misc-fixes
~~~~~~~~~~~~~~

This is for bugfixes which target the current -rc cycle.

.. _drm-intel-repository:

The Upstream i915 Driver Repository
-----------------------------------

See drm-intel_ for repository details. Maintained by Jani Nikula, Joonas
Lahtinen and Rodrigo Vivi, with a large pool of committers. Consists mostly of
``drivers/gpu/drm/i915``.

.. _drm-intel: https://cgit.freedesktop.org/drm/drm-intel

drm-intel-next-queued (aka "dinq")
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the branch where all new features, as well as any non-trivial or
controversial fixes, are applied.

This branch "hides" the merge window from the drm/i915 developers; patches are
applied here regardless of the development phase of Linus' upstream kernel.

drm-intel-next
~~~~~~~~~~~~~~

drm-intel-next-queued at some point in time.

drm-intel-next-fixes (aka "dinf")
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This branch contains drm/i915 specific fixes to drm-next after the drm/i915
features have been merged there. Fixes are first applied to
drm-intel-next-queued, and cherry-picked to drm-intel-next-fixes.

Pull requests to Dave are sent as needed, with no particular schedule.

drm-intel-fixes (aka "-fixes")
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This branch contains fixes to Linus' tree after drm-next has been merged during
the merge window. Fixes are first applied to drm-intel-next-queued, and
cherry-picked to drm-intel-fixes. The fixes are then merged through drm-fixes.
Valid from -rc1 to the kernel release.

Usually Linus releases each -rc on a Sunday, and drm-intel-fixes gets rebased on
that the following Monday. Usually this is a fast-forward. The pull request to
Dave for new fixes is typically sent on the following Thursday. This is repeated
until final release of the kernel.

This is the fastest path to getting fixes to Linus' tree. It is generally for
the regressions, cc:stable, black screens, GPU hangs only, and should pretty
much follow the stable rules.

The DRM Testing and Integration Repository
------------------------------------------

See drm-tip_ for repository details.

.. _drm-tip: https://cgit.freedesktop.org/drm/drm-tip

drm-tip
~~~~~~~

This is the overall integration tree for drm, and lives in
``git://anongit.freedesktop.org/drm-tip``. Every time one of the above branches
is updated drm-tip gets rebuilt. If there's a conflict see section on `resolving
conflicts when rebuilding drm-tip
<drm-intel.html#resolving-conflicts-when-rebuilding-drm-tip>`_.

drm-rerere
~~~~~~~~~~

This branch contains the `nightly.conf`_ configuration file and the shared ``git
rerere`` conflict resolutions for dim to generate drm-tip, as well as some
kernel defconfig files for build testing.

.. _nightly.conf: https://cgit.freedesktop.org/drm/drm-tip/plain/nightly.conf?h=rerere-cache

maintainer-tools
~~~~~~~~~~~~~~~~

This branch contains all the tools and documentation you're reading about.
