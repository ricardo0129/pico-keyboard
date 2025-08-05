pipeline {
    agent {
        kubernetes {
            label 'cpp-build-agent'
            yaml """
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: build
    image: ricky0129/pico-sdk
    command:
    - cat
    tty: true
"""
        }
    }
    stages {
        stage('Build') {
            steps {
                container('build') {
                    sh 'cmake -S . -B build -DPICO_BOARD=pico2_w'
                    sh 'cmake --build build'
                }
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

    
    post {
        success {
            archiveArtifacts artifacts: 'build/main.uf2', fingerprint: true
        }
    }
}
