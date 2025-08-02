pipeline {
    agent any // or specify a specific agent, like 'agent { label 'my-agent' }'

    stages {
        stage('Build') {
            steps {
                echo "Building the application..."
                echo "Checking out the code from the repository..."
            }
        }
        stage('Test') {
            steps {
                echo "Running tests..."
                // Add test commands here, e.g., sh 'npm test' or sh './gradlew test'
            }
        }
        stage('Deploy') {
            when {
                branch 'main' // Deploy only for the 'main' branch
            }
            steps {
                echo 'Deploying application...'
                // Add deployment steps here
            }
        }
    }
}
