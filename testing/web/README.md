# Example


See: http://libreswan.org/results/


# Manual Setup


To publish the results from running `make kvm-results` as a web page,
point the make variable `LSW_WEBDIR` at the web page's HTML directory.
For instance, either by adding it to Makefile.inc.local:

   $ mkdir /var/www/html/results
   echo LSW_WEBDIR=/var/www/html/results >> Makefile.inc.local

or your .profile:

   $ echo export LSW_WEBDIR=/var/www/html/results >> ~/.profile



# Nuances


Some things, annoyingly, don't quite work right:

- comparisons sometimes loose a result

  The code fetching all the json result tables is likely racy, when
  too may results are clicked too fast a result goes missing.  The
  work around is to de-select and then re-select the missing result.

- libreswan's repo contains "knots"

  When following a linear history (parent links), the commit dates
  normally decrease.  In this repo they some times increase resulting
  in graph doubling back on itself.

- libreswan's repo contains "plats"

  As a generalization, is good to merge local changes onto the remote
  first-parent branch and not the reverse.  Unfortunately `git pull
  (git merge)` does the reverse by default.  The result is that
  first-parent keeps switching with development branches.

- clicking links toggls a results selection

  For instance, control-click a result's subject hash link (to open a
  new tab displaying the diff)) also toggls the results appearance
  under the comparison tab



# Improvements


Some things could work better:

- "good" and "wip" each list their errors separately

  So it is easier to identify "good" errors.

  Unfortunately, the raw data (found in `results.json`) isn't
  currently included in `summary.json`.

- examine (compare) individual tests

  For instance, select a test in the "Compare Results" tab would
  display (graph?) that test's history under a "Compare Test" tab.

  The raw test results are in <run>/<test>/OUTPUT/result.json.

- Bookmark comparisons

  For instance `results?hash,hash`.

  The required information seems to be in `location.search`.

- Use an accumulative bar graph instead of a scatter plot

  It probably better represents the data.  However, since it needs to
  also plot branches things could get interesting.

- Graph should pan and zoom

- trim older directories so the total is kept reasonable

- use rowspan=... in the summary table


# Automated Testing


## Setup


Automated testing requires at least two libreswan repositories:

- the repository under test (aka slave) - required

  In addition to regular updates using "git fetch", this repository is
  switched to the commit being tested using "git reset --hard".

  Create a top-level Makefile.inc.local file in this directory to
  configure KVM_WORKERS and KVM_PREFIXES as required.

- the repository containing the web sources and scripts (aka master) -
  required

- the repository for rebuilding the web site (aka scratch) - optional

  When re-generating the results from a test run (for instance as part
  of rebuilding the web-site after a json file format change), this
  repository is "git reset --hard" to the original commit used to
  generate those results.

for instance:

    $ git clone git@github.com:libreswan/libreswan.git libreswan-web-slave
    $ git clone git@github.com:libreswan/libreswan.git libreswan-web-master
    $ git clone git@github.com:libreswan/libreswan.git libreswan-web-scratch


## Running


Assuming results are to be published in /var/www/html/results, the
testing script is invoked as:

    $ nohub ./libreswan-web-master/testing/web/tester.sh libreswan-web-slave /var/www/html/results/ &
    $ tail -f nohup.out


## Maintenance


From time to time the test results are seen to decay.  The symptom is
an increasing number of test failures with a result of "unresolved"
and an error of "output-missing".  The error occures because one or
more test domains fail to start in a timely manner (the test runner
gives up after two minutes), hence, while the test is processed there
is never any output.

Per the below, the best workaround seems to be to stop the testing
script and then re-build the test domains.  The alternative - rebuild
the test domains from the base domain at the start of each new test
run - is even less reliable!  Perhaps tester.sh should be modified to
automate the below.

- (optional, but a good idea) upgrade and reboot the test machine:

      $ sudo dnf upgrade -y
      $ sudo reboot

- delete the existing test domains domains (leaving the base domain,
  this should crash the current test run):

      $ ( cd libreswan-web-slave && make kvm-uninstall )

- update the slave repository:

      $ ( cd libreswan-web-slave && git pull --ff-only )

- update the base domain:

      $ ( cd libreswan-web-slave && make kvm-upgrade-base-domain )

- update the master repository:

      $ ( cd libreswan-web-master && git pull --ff-only )

- examine (and perhaps delete) any test runs where tests have
  'missing-output':

      $ grep '"output-missing"' "${WEBDIR}"/*-g*-*/results.json | cut -d/ -f1 | sort -u

- examine (and perhaps delete) test runs with no results.json:

      $ ls -d "${WEBDIR}"/*-g*-*/ | while read d ; do test -r $d/results.json || echo $d ; done

- examine (and perhaps delete) a random selection of test runs:

      # form a list of non-branch et.al. test runs
      $ ./libreswan-web-master/testing/web/gime-work.sh "${WEBDIR}" libreswan-web-slave 2>&1 | grep -e '^tested:[^:]*$' | tee tested.tmp
      # ignoring the first, select a random subset
      $ tail -n +2 tested.tmp | shuf | tail -n +100 | tee /dev/stderr | while read a h b ; do echo "${WEBDIR}"/*$h* ; done

  (gime-work.sh lists each test result, and how "interesting" it was,
  on stderror)

- restart <tt>tester.sh</tt>:

      $ nohup ./libreswan-web-master/testing/web/tester.sh libreswan-web-slave "${WEBDIR}" &


## Rebuilding


From time-to-time the web site may require a partial or full rebuild.

For HTML (.html, .css and .js) files, the process is straight forward.
However, for the .json files, the process can be slow (and in the case
of the results, a dedicated git repository is needed).

- `make web [WEB_SCRATCH_REPODIR=.../libreswan-web-scratch]`

  Update the web site.

  If WEB_SCRATCH_REPODIR is specified, then the result.json files in
  the test run sub-directories under $(WEB_SUMMARYDIR) are also
  updated.

- `make web-results-html`

  Update any out-of-date HTML (.html, .css and .json) files in the
  results sub-directories.

  Touching the source file `testing/web/results.html` will force an
  update.

- `make web-commits-json`

  Update the commits.json file which contains a table of all the
  commits.  Slow.

  Touching the script `testing/web/json-commit.sh`, which is used to
  create the files, will force an update.  So too will deleting the
  .../commits/ directory.

- `make web-results-json WEB_SCRATCH_REPODIR=.../libreswan-web-scratch`

  Update the `results.json` file in each test run's sub-directory.
  Very slow.  Requires a dedicated git repository.

  Touching the script `testing/utils/kvmresults.py`, which is used to
  generate results.json, will force an update.

- `make '$(WEB_SUMMARYDIR)/<run>/results.json' WEB_SCRATCH_REPODIR=.../libreswan-web-scratch`

  Update an individual test run's `results.json` file.  Slow.
  Requires a dedicated git repository.


# References
