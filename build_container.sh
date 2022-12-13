PLATFORMS="linux/arm64"
docker buildx build --platform="${PLATFORMS}" --push -t docker.io/bniemczyk/bnirc .
