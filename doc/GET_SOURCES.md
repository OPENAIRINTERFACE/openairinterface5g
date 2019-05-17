<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">The OpenAirInterface repository: the sources</font></b>
    </td>
  </tr>
</table>

The OpenAirInterface software can be obtained from our gitLab
server. You will need a git client to get the sources. The repository
is currently used for main developments.

# Prerequisites

You need to install the subversion/git using the following commands:

```shell
sudo apt-get update
sudo apt-get install subversion git
```

# Using EURECOM Gitlab

The [openairinterface5g repository](https://gitlab.eurecom.fr/oai/openairinterface5g.git)
holds the source code for (eNB RAN + UE RAN).

For legal issues (licenses), the core network (EPC) source code is now moved away from
the above openairinterface5g git repository.

*  A very old version of the EPC is located under the same GitLab Eurecom server (splitted into 2 Git repos):
   *  [openair-cn](https://gitlab.eurecom.fr/oai/openair-cn.git) with apache license
   *  [xtables-addons-oai](https://gitlab.eurecom.fr/oai/xtables-addons-oai.git) with GPL license
   *  **These repositories are no more maintained.**
*  A more recent version is available under our GitHub domain:
   *  [OAI GitHub openair-cn](https://github.com/OPENAIRINTERFACE/openair-cn)
   *  Check its wiki pages for more details

Configure git with your name/email address (only important if you are developer and want to contribute/push code to Git Repository):

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

## In order to clone the Git repository (for OAI Users without login to gitlab server)

Cloning RAN repository (eNB RAN + UE RAN):

```shell
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
```

## In order to contribute to the Git RAN repository (for OAI Developers/admins with login to gitlab server)

Please send email to [contact@openairinterface.org](mailto:contact@openairinterface.org) to be added to the repository
as a developer (only important for users who want to commit code to the repository). If
you do not have account on gitlab.eurecom.fr, please register yourself to gitlab.eurecom.fr and provide the identifiant in the email.

* Clone with using ssh keys:
   * You will need to put your ssh keys in https://gitlab.eurecom.fr/profile/keys
     to access to the git repo. Once that is done, clone the git repository using:
   * `git clone git@gitlab.eurecom.fr:oai/openairinterface5g.git`
* Clone with user name/password prompt:
   * `git clone https://YOUR_USERNAME@gitlab.eurecom.fr/oai/openairinterface5g.git`
   * `git clone https://YOUR_USERNAME@gitlab.eurecom.fr/oai/openair-cn.git`
   * `git clone https://YOUR_USERNAME@gitlab.eurecom.fr/oai/xtables-addons-oai.git` (optional, openair-cn build script can do it for you)

# Which branch to checkout?

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

