pipeline {
  agent any
  stages {
    stage('pull') {
      steps {
        sh 'echo "u-boot pipeline test"'
      }
    }
    stage('') {
      steps {
        setGerritReview()
        setGerritReview(customUrl: 'review.gerrithub.io', unsuccessfulMessage: 'abc')
      }
    }
  }
}
