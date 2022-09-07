The OpenAirInterface software can be obtained from our gitLab
server. You will need a git client to get the sources. The repository
is currently used for main developments.

# Prerequisites

You need to install git using the following commands:

```shell
sudo apt-get update
sudo apt-get install git
```

# Using EURECOM Gitlab

The [openairinterface5g repository](https://gitlab.eurecom.fr/oai/openairinterface5g.git)
holds the source code for the RAN (4G and 5G).

Configure git with your name/email address (only important if you are developer and want to contribute/push code to Git Repository):

```shell
git config --global user.name "Your Name"
git config --global user.email "Your email address"
```

## In order to clone the Git repository (for OAI Users without login to gitlab server)

Cloning RAN repository (eNB RAN + UE RAN):

```shell
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
```

## In order to contribute to the Git RAN repository (for OAI Developers/admins with login to gitlab server)

Please send email to [contact@openairinterface.org](mailto:contact@openairinterface.org) to be added to the repository
as a developer (only important for users who want to commit code to the repository). If
you do not have account on gitlab.eurecom.fr, please register yourself to gitlab.eurecom.fr and provide the identifiant in the email.

# Which branch to checkout?

On the RAN side:

* **master**: This branch is targeted for the user community. Since January 2019, it is also subject to a Continuous Integration process. The update frequency is about once every 2-3 months. We are also performing bug fixes on this branch.
* **develop**: This branch contains recent commits that are tested on our CI test bench. The update frequency is about once a week.

Please see the work flow and policies page:

https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/oai-policies-home

you can find the latest stable tag release here:

https://gitlab.eurecom.fr/oai/openairinterface5g/tags

The tag naming conventions are:

- On `master` branch: **v1.`x`.`y`** where
  * `x` is the minor release number, incremented every 2-3 months when we are merging `develop` into `master` branch.
  * `y` is the maintenance number, starting at 0 when we do a minor release and being incremented when a bug fix is incorporated into `master` branch.
- On `develop` branch **`yyyy`.w`xx`**
  * `yyyy` is the calendar year
  * `xx` the week number within the year

