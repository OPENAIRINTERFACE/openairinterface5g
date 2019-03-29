# The OpenAirInterface repository

The OpenAirInterface software can be obtained from our gitLab
server. You will need a git client to get the sources. The repository
is currently used for main developments.

## Prerequisite

You need to install the subversion/git using the following commands:

```shell
sudo apt-get update
sudo apt-get install subversion git
```

## Using EURECOM Gitlab

The [openairinterface5g repository](https://gitlab.eurecom.fr/oai/openairinterface5g.git)
holds the source code for (eNB RAN + UE RAN).

For legal issues (licenses), the core network (EPC) source code is now moved away from
the above openairinterface5g git repository. This EPC code is now splitted into two git
projects ([openair-cn](https://gitlab.eurecom.fr/oai/openair-cn.git) with apache license
and [xtables-addons-oai](https://gitlab.eurecom.fr/oai/xtables-addons-oai.git) with GPL license).

Configure git with your name/email address (only important if you are developer and want to checkin code to Git):

```shell
git config --global user.name "Your Name"
git config --global user.email "Your email address"
```

- Add a certificate from gitlab.eurecom.fr to your Ubuntu 14.04 installation:

```shell
echo -n | openssl s_client -showcerts -connect gitlab.eurecom.fr:443 2>/dev/null | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' | sudo tee -a /etc/ssl/certs/ca-certificates.crt
```

- Disable certificate check completely if you do not have root access to /etc/ssl directory

```shell
git config --global http.sslverify false
```

### In order to checkout the Git repository (for OAI Users without login to gitlab server)

Checkout RAN repository (eNB RAN + UE RAN):

```shell
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
```

Checkout EPC (Core Network) repository:

```shell
git clone https://gitlab.eurecom.fr/oai/openair-cn.git
```

Optionally (openair-cn build script can install it for you):

```shell
git clone https://gitlab.eurecom.fr/oai/xtables-addons-oai.git
```

### In order to checkout the Git repository (for OAI Developers/admins with login to gitlab server)

Please send email to {openair_tech (AT) eurecom (DOT) fr} to be added to the repository
as a developer (only important for users who want to commit code to the repository). If
you do not have account on gitlab.eurecom.fr, please register yourself to gitlab.eurecom.fr.

* Checkout with using ssh keys:
   * You will need to put your ssh keys in https://gitlab.eurecom.fr/profile/keys
     to access to the git repo. Once that is done, checkout the git repository using:
   * `git clone git@gitlab.eurecom.fr:oai/openairinterface5g.git`
* Checkout with user name/password prompt:
   * `git clone https://YOUR_USERNAME@gitlab.eurecom.fr/oai/openairinterface5g.git`
   * `git clone https://YOUR_USERNAME@gitlab.eurecom.fr/oai/openair-cn.git`
   * `git clone https://YOUR_USERNAME@gitlab.eurecom.fr/oai/xtables-addons-oai.git` (optional, openair-cn build script can do it for you)

## Which branch to checkout?

On the RAN side:

* **master**: This branch is targeted for the user community. Since January 2019, it is also subject to a Continuous Integration process. The update frequency is about once every 2-3 months. We are also performing bug fixes on this branch.
* **develop**: This branch contains recent commits that are tested on our CI test bench. The update frequency is about once a week.

Please see the work flow and policies page :

https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/oai-policies-home

you can find the latest stable tag release here :

https://gitlab.eurecom.fr/oai/openairinterface5g/tags

The tag naming conventions are :

- On `master` branch: **v1.`x`.`y`** where
  * `x` is the minor release number, incremented every 2-3 months when we are merging `develop` into `master` branch.
  * `y` is the maintenance number, starting at 0 when we do a minor release and being incremented when a bug fix is incorporated into `master` branch.
- On `develop` branch **`yyyy`.w`xx`**
  * `yyyy` is the calendar year
  * `xx` the week number within the year

