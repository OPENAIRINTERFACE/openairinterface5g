This file describes briefly the steps that were carried out to migrate from *PUBLIC* OAI SVN repo (http://svn.eurecom.fr/openair4G/trunk) to the gitlab repo (git@gitlab.eurecom.fr:oai/openairinterface5G.git). In order to reduce the size of repository, we have only included SVN commits from r4000 up till now. None of branches/tags are migrated from SVN Repository to Git. 

1. The steps to migrate from SVN to Git were carried out using the instructions/script here, https://www.atlassian.com/git/tutorials/migrating-overview

2. Currently, the repository is public during the "transition period" for everyone to get used to the repository. During the "transition period", we will check in changes only to SVN and synchronize SVN to Git repository once everyday. At the end of "transition period", we will mark SVN repo as read-only and Git repository will be the default working repository. Currently, the "transition period" is set to 1 month until 15 July. However, this date might be changed based on the feedback from the community.There are brief instructions to explain this workflow here, https://www.atlassian.com/git/tutorials/migrating-overview/.

3. The author list file is located in authors.txt in the main working directory. This file was extracted from SVN logs, but is modified for name and email addresses. 

4. Add a certificate from gitlab.eurecom.fr to your Ubuntu 14.04 installation (you need to be root user)
   a) echo -n | openssl s_client -showcerts -connect gitlab.eurecom.fr:443 2>/dev/null  | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' > /usr/local/share/ca-certificates/gitlab_eurecom_fr.crt
   b)update-ca-certificates

5. Git one time configuration with your name/email address.
   git config --global user.name "Your Name"
   git config --global user.email "Your email address"

6. In order to checkout the Git repository (for OAI Users without login to gitlab server)
   a) export GIT_SSL_NO_VERIFY=1; git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git (without certificate installation in step 4)
   b) git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git (If certificate is installed in step 4)

6. In order to checkout the Git repository (for OAI Developers/admins with login to gitlab server). Please send email to openair_tech@eurecom.fr to be added to the repository as a developer). If you do not have account on gitlab.eurecom.fr, please register yourself to gitlab.eurecom.fr.

  a) Checkout with using ssh keys:
     You will need to put your ssh keys in https://gitlab.eurecom.fr/profile/keys to access to the  git repo (only important for users who want to commit to the repository). Once  that is done, checkout the git repository using:
   git clone git@gitlab.eurecom.fr:oai/openairinterface5g.git

  b) Checkout with user name/password prompt:
   
   git clone https://YOUR_USERNAME@gitlab.eurecom.fr/oai/openairinterface5g.git



Migration Instructions from SVN->Git in Detail (Not important for OAI Developer Community):
Assuming you are in svn2git directory.

wget https://bitbucket.org/atlassian/svn-migration-scripts/downloads/svn-migration-scripts.jar
java -jar ./svn-migration-scripts.jar verify
java -jar ./svn-migration-scripts.jar authors http://svn.eurecom.fr/openair4G > authors.txt
git svn clone -r4000:HEAD --authors-file=./svn2git/authors.txt http://svn.eurecom.fr/openair4G/trunk oaiRepoData1 >& log_svn_clone1.out

(Assuming you have added your ssh keys in https://gitlab.eurecom.fr/profile/keys)
//One time git configuration of user name/email address
git config --global user.name "Replace_With_Your_Name"
git config --global user.email "Replace_With_Your_Email_address"

git remote add origin git@gitlab.eurecom.fr:oai/openairinterface5g.git

git push -u origin --all
git push --tags

Steps to synchronize on calisson (to be carried out periodically)
cd ~rohit/GitMigration/GitRepoNew/oaiRepoData1/svn2git
git config -l
git config --global user.name "Replace_With_Your_Name"
git config --global user.email "Replace_With_Your_Email_address"
./check_diff_author.sh #There should be no difference in author list file
git remote set-url origin 	https://kaltenbe@gitlab.eurecom.fr/oai/openairinterface5g.git 
git svn fetch #--authors-file=./authors.txt (Make sure that there are no new authors since the last migration, else this will result in errors)
git svn rebase
git push -u origin --all
git push --tags



