This php script has been lifted from [@squix78's project](https://github.com/squix78/esp8266-ci-ota), see [blog post](http://blog.squix.org/2016/06/esp8266-continuous-delivery-pipeline-push-to-production.html). Thanks a lot to Daniel Eichhorn for developing the ESP OTA CONTINUOUS DELIVERY workflow.

From @squi78's blog:

> ## The PHP script

> It would have been nice if the production ESPs could have contacted the Github API directly. But there are two issues that made be use the intermediate PHP script:

> - The ESPhttpUpdate currently cannot follow redirects. This is important since github hosts the release artefacts on Amazon AWS. But in the API JSON > object the address points to github, so the http client has to follow a redirect to download the artefact.
> - Github uses https for its API and will redirect you to it if you are trying plain HTTP. This means that you would have to know the SSL fingerprints > of the github API server and the AWS hosting instance since this is required by the ESPs secure client interface. After all the ESPs don’t have a chain > of trusted certificates stored somewhere. While the fingerprint of the github API might be stable, the redirection on Amazon AWS might not always use > the same certificate.
>
> So what does the script do?
>
> It connects to the github API and fetches a JSON object describing the latest release. I’m currently only interested in the tag name and the firmware > download URL. The ESPs will send their firmware tag version with the update request and if the latest tag on Github and the one from the request are > identical nothing will happen. But if they are different the script will fetch the binary from github (with a hidden redirection to Amazon) and proxy > it to the ESP’s requesting it.