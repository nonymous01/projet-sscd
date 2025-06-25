Memo: Create and configure AWS ECR + Docker build & push

# 1. Connexion à AWS
aws configure
# Saisir les credentials AWS (Access Key ID, Secret Access Key, Region, Output format)

# 2. Créer un dépôt ECR
aws ecr create-repository --repository-name nom-depot --region us-east-1

# 3. Connexion Docker à ECR (remplace REGION par ta région ex: us-east-1)
aws ecr get-login-password --region us-east-1 | docker login --username AWS --password-stdin <account_id>.dkr.ecr.us-east-1.amazonaws.com

# 4. Build de l’image Docker
docker build -t nom-image .

# 5. Tag de l’image pour ECR
docker tag nom-image:latest <account_id>.dkr.ecr.us-east-1.amazonaws.com/nom-depot:latest

# 6. Push de l’image dans ECR
docker push <account_id>.dkr.ecr.us-east-1.amazonaws.com/nom-depot:latest
