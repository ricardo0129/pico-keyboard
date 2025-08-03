pipeline {
    agent any
    tools {
        cmake 'cmake-3.28'
    }
    stages {
        stage('Build') {
            steps {
                sh 'cmake -S . -B build -DPICO_BOARD=pico2_w'
                sh 'cmake --build build'
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
